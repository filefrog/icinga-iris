#include "tap.c"
#include "../iris.h"

#define NET_HOST "127.0.0.1"
#define NET_PORT "12358"

int expect_packets = 0;
int num_packets    = 0;

int child_main(int fd)
{
	int i;
	size_t n, len;
	struct pdu pdu;
	time_t now;

	strcpy(pdu.host,    "a.host");
	strcpy(pdu.service, "b.service");
	strcpy(pdu.output,  "output");

	for (i = 0; i < expect_packets; i++) {
		pdu.rc = i % 3;
		time(&now); pdu.ts = (uint32_t)now;
		if (pdu_pack(&pdu) != 0) return 2;

		len = pdu_write(fd, (char*)&pdu);
		if (len < sizeof(pdu)) return 3;

		pdu.host[0]++;
		pdu.service[0]++;
	}

	return 0;
}

// test evhandler
void test_submit(struct pdu *pdu)
{
	ok(pdu->rc == num_packets % 3, "Got packet with RC %d (expect %d)",
			pdu->rc, num_packets % 3);
	num_packets++;
}

int main(int argc, char **argv)
{
	int pipefd[2];

	plan_no_plan();
	freopen("/dev/null", "w", stderr);

	ok(pipe(pipefd) == 0, "got a bi-directional pipe");
	ok(nonblocking(pipefd[0]) == 0, "set O_NONBLOCK on read end of pipe");
	num_packets = 0;
	expect_packets = 1;
	if (fork() == 0) {
		close(pipefd[0]);
		_exit(child_main(pipefd[1]));
	}
	close(pipefd[1]);
	recv_data(pipefd[0], test_submit);
	ok(num_packets == expect_packets, "received all %d packets (expected %d)",
			num_packets, expect_packets);
	close(pipefd[0]);


	ok(pipe(pipefd) == 0, "got a bi-directional pipe");
	ok(nonblocking(pipefd[0]) == 0, "set O_NONBLOCK on read end of pipe");
	num_packets = 0;
	expect_packets = 6;
	if (fork() == 0) {
		close(pipefd[0]);
		_exit(child_main(pipefd[1]));
	}
	close(pipefd[1]);
	recv_data(pipefd[0], test_submit);
	ok(num_packets == expect_packets, "received all %d packets (expected %d)",
			num_packets, expect_packets);
	close(pipefd[0]);

	return exit_status();
}
