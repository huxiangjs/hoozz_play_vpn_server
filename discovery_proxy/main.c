#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define ARRAY_SIZE(arr)			(sizeof(arr) / sizeof((arr)[0]))

#define DISCOVERY_PORT			54542
#define DISCOVERY_SAY			"HOOZZ?"
#define DISCOVERY_RESPOND		"HOOZZ:"
#define DISCOVERY_BROADCAST_ADDRESS	"255.255.255.255"

#define VPN_EXTEND_LIFE			20
#define LAN_EXTEND_LIFE			10
#define LAN_REFRESH_INTERVAL		5

#define CONTENT_SIZE_MAX		256

struct vpn_device {
	int life;
	char ip[16];
};

struct lan_device {
	int life;
	char ip[16];
	char content[CONTENT_SIZE_MAX];
};

static int vpn_sockfd = -1;
static int lan_sockfd = -1;

static struct vpn_device vpn_list[256];
static struct lan_device lan_list[256];

static pthread_mutex_t vpn_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lan_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t refresh_mutex = PTHREAD_MUTEX_INITIALIZER;

static int lan_refresh_life;

static void *vpn_if_thread(void *p)
{
	const char *ip_addr = (const char *)p;
	int sockfd;
	struct sockaddr_in servaddr, cliaddr, tmpaddr;
	char buffer[128];
	char content[CONTENT_SIZE_MAX + 16];
	int len, n, t, i;
	int operate = 1;
	char *remote_ip;
	int remote_port;
	int idle_index;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &operate, sizeof(operate));
	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &operate, sizeof(operate));

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(DISCOVERY_PORT);
	if (inet_pton(AF_INET, ip_addr, &servaddr.sin_addr) <= 0) {
		printf("Invalid address: %s\n", ip_addr);
		exit(EXIT_FAILURE);
	}

	memset(&tmpaddr, 0, sizeof(tmpaddr));
	tmpaddr.sin_family = AF_INET;
	tmpaddr.sin_port = htons(DISCOVERY_PORT);
	if (inet_pton(AF_INET, DISCOVERY_BROADCAST_ADDRESS, &tmpaddr.sin_addr) <= 0) {
		printf("Invalid address: %s\n", DISCOVERY_BROADCAST_ADDRESS);
		exit(EXIT_FAILURE);
	}

	if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("Bind failed");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	vpn_sockfd = sockfd;

	len = sizeof(struct sockaddr_in);
	while (1) {
		n = recvfrom(sockfd, buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *)&cliaddr, (socklen_t*)&len);
		buffer[n == sizeof(buffer) ? n - 1 : n] = '\0';
		remote_ip = inet_ntoa(cliaddr.sin_addr);
		remote_port = ntohs(cliaddr.sin_port);

		if (!memcmp(DISCOVERY_SAY, buffer, sizeof(DISCOVERY_SAY) - 1)) {
			printf("VPN %s:%d say %s\n", remote_ip, remote_port, buffer);

			pthread_mutex_lock(&vpn_mutex);
			idle_index = -1;
			for (i = 0; i < ARRAY_SIZE(vpn_list); i++) {
				if (!vpn_list[i].life) {
					if (idle_index < 0)
						idle_index = i;
					continue;
				}
				if (!strcmp(remote_ip, vpn_list[i].ip)) {
					vpn_list[i].life = VPN_EXTEND_LIFE;
					idle_index = -1;
					break;
				}
			}

			if (idle_index >= 0) {
				vpn_list[idle_index].life = VPN_EXTEND_LIFE;
				strcpy(vpn_list[idle_index].ip, remote_ip);
				printf("VPN device %s active\n", remote_ip);
			}
			pthread_mutex_unlock(&vpn_mutex);

			pthread_mutex_lock(&refresh_mutex);
			if (lan_sockfd >= 0 && !lan_refresh_life) {
				sendto(lan_sockfd, buffer, strlen(buffer), MSG_CONFIRM, (struct sockaddr *)&tmpaddr, len);
				lan_refresh_life = LAN_REFRESH_INTERVAL;
				printf("Discovery resume\n");
			}
			pthread_mutex_unlock(&refresh_mutex);

			/* First send the cached lan device */
			pthread_mutex_lock(&lan_mutex);
			for (i = 0; i < ARRAY_SIZE(lan_list); i++) {
				if (!lan_list[i].life)
					continue;
				n = strlen(lan_list[i].content);
				memcpy(content, lan_list[i].content, n);
				content[n] = '\0';
				n++;
				t = strlen(lan_list[i].ip);
				memcpy(content + n, lan_list[i].ip, t);
				sendto(sockfd, content, n + t, MSG_CONFIRM, (struct sockaddr *)&cliaddr, len);
			}
			pthread_mutex_unlock(&lan_mutex);
		}
	}

	close(sockfd);

	// pthread_exit(NULL);
	exit(EXIT_FAILURE);
}

static void *lan_if_thread(void *p)
{
#if 0
	const char *ip_addr = (const char *)p;
#endif
	int sockfd;
	struct sockaddr_in servaddr, cliaddr, tmpaddr;
	char buffer[CONTENT_SIZE_MAX];
	char content[CONTENT_SIZE_MAX + 16];
	int len, n, i, t;
	int operate = 1;
	char *remote_ip;
	int remote_port;
	int idle_index;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &operate, sizeof(operate));
	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &operate, sizeof(operate));

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(DISCOVERY_PORT);
#if 0
	if (inet_pton(AF_INET, ip_addr, &servaddr.sin_addr) <= 0) {
		printf("Invalid address: %s\n", ip_addr);
		exit(EXIT_FAILURE);
	}
#else
	/* This will receive a broadcast with the destination address of 255.255.255.255 */
	servaddr.sin_addr.s_addr = INADDR_ANY;
#endif

	if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("Bind failed");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	lan_sockfd = sockfd;

	len = sizeof(struct sockaddr_in);
	while (1) {
		n = recvfrom(sockfd, buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *)&cliaddr, (socklen_t*)&len);
		n = n == sizeof(buffer) ? n - 1 : n;
		buffer[n] = '\0';
		remote_ip = inet_ntoa(cliaddr.sin_addr);
		remote_port = ntohs(cliaddr.sin_port);

		if (!memcmp(DISCOVERY_RESPOND, buffer, sizeof(DISCOVERY_RESPOND) - 1)) {
			printf("LAN %s:%d say %s\n", remote_ip, remote_port, buffer);

			pthread_mutex_lock(&lan_mutex);
			idle_index = -1;
			for (i = 0; i < ARRAY_SIZE(lan_list); i++) {
				if (!lan_list[i].life) {
					if (idle_index < 0)
						idle_index = i;
					continue;
				}
				if (!strcmp(remote_ip, lan_list[i].ip)) {
					lan_list[i].life = LAN_EXTEND_LIFE;
					idle_index = -1;
					break;
				}
			}

			if (idle_index >= 0) {
				lan_list[idle_index].life = LAN_EXTEND_LIFE;
				strcpy(lan_list[idle_index].ip, remote_ip);
				strcpy(lan_list[idle_index].content, buffer);
				printf("LAN device %s active\n", remote_ip);
			}
			pthread_mutex_unlock(&lan_mutex);

			/* Send directly to active vpn device */
			pthread_mutex_lock(&vpn_mutex);
			memcpy(content, buffer, n);
			content[n] = '\0';
			n++;
			t = strlen(remote_ip);
			memcpy(content + n, remote_ip, t);
			n += t;
			for (i = 0; i < ARRAY_SIZE(vpn_list); i++) {
				if (!vpn_list[i].life)
					continue;
				memset(&tmpaddr, 0, sizeof(tmpaddr));
				tmpaddr.sin_family = AF_INET;
				tmpaddr.sin_port = htons(DISCOVERY_PORT);
				if (inet_pton(AF_INET, vpn_list[i].ip, &tmpaddr.sin_addr) <= 0) {
					printf("Invalid address: %s\n", vpn_list[i].ip);
					continue;
				}
				sendto(sockfd, content, n, MSG_CONFIRM, (struct sockaddr *)&tmpaddr, len);
			}
			pthread_mutex_unlock(&vpn_mutex);
		}
	}

	close(sockfd);

	// pthread_exit(NULL);
	exit(EXIT_FAILURE);
}

static void *degeneration_thread(void *p)
{
	int i;

	while (1) {
		pthread_mutex_lock(&vpn_mutex);
		for (i = 0; i < ARRAY_SIZE(vpn_list); i++) {
			if (!vpn_list[i].life)
				continue;
			vpn_list[i].life--;
			if (!vpn_list[i].life)
				printf("VPN device %s died\n", vpn_list[i].ip);
		}
		pthread_mutex_unlock(&vpn_mutex);

		pthread_mutex_lock(&lan_mutex);
		for (i = 0; i < ARRAY_SIZE(lan_list); i++) {
			if (!lan_list[i].life)
				continue;
			lan_list[i].life--;
			if (!lan_list[i].life)
				printf("LAN device %s died\n", lan_list[i].ip);
		}
		pthread_mutex_unlock(&lan_mutex);

		pthread_mutex_lock(&refresh_mutex);
		if (lan_refresh_life) {
			lan_refresh_life--;
			if (!lan_refresh_life)
				printf("Discovery suspend\n");
		}
		pthread_mutex_unlock(&refresh_mutex);

		sleep(1);
	}

	// pthread_exit(NULL);
	exit(EXIT_FAILURE);
}

int main(int args, const char *argv[])
{
	const char *lan_ip;
	const char *vpn_ip;
	pthread_t threads[3];
	int ret;
	int t;

	printf("@@@@@@@@@@@@@@ Discover Proxy @@@@@@@@@@@@@@\n");

	if (args != 3) {
		printf("Wrong input parameters\n");
		exit(EXIT_FAILURE);
	}
	vpn_ip = argv[1];
	lan_ip = argv[2];

	ret = pthread_create(&threads[0], NULL, vpn_if_thread, (void *)vpn_ip);
	if (ret) {
		printf("Error: unable to create thread[0]: %d\n", ret);
		exit(EXIT_FAILURE);
	}

	ret = pthread_create(&threads[1], NULL, lan_if_thread, (void *)lan_ip);
	if (ret) {
		printf("Error: unable to create thread[1]: %d\n", ret);
		exit(EXIT_FAILURE);
	}

	ret = pthread_create(&threads[2], NULL, degeneration_thread, NULL);
	if (ret) {
		printf("Error: unable to create thread[2]: %d\n", ret);
		exit(EXIT_FAILURE);
	}

	for(t = 0; t < ARRAY_SIZE(threads); t++)
		pthread_join(threads[t], NULL);

	printf("Exited\n");

	return 0;
}

