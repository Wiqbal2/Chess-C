
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define DEVICE_PATH "/dev/chessgame"

int main() {
    int fd;
    char command[256];
    ssize_t bytes;
    char read_buf[1024]; // Buffer to store read data
    ssize_t num_read;
    // Open the device file for reading and writing
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Failed to open the device '%s': %s\n", DEVICE_PATH, strerror(errno));
        return EXIT_FAILURE;
    }

    printf("Chess game started. Enter commands to play.\n");
    printf("Commands:\n");
    printf("  00 W/B - Start new game as White (W) or Black (B)\n");
    printf("  01 - Get current game state\n");
    printf("  02 [move] - Make a move (e.g., WPe2-e4xP)\n");
    printf("  03 - CPU makes a move\n");
    printf("  04 - Resign game\n");
    printf("Type 'exit' to quit.\n");

    while (1) {
        printf("\nEnter command: ");
        if (!fgets(command, sizeof(command), stdin)) {
            perror("Failed to read command");
            continue;
        }

        if (strncmp(command, "exit", 4) == 0) {
            break;
        }

        // Remove newline character at the end if present
        command[strcspn(command, "\n")] = 0;

        // Send command to the device
        bytes = write(fd, command, strlen(command));
        if (bytes < 0) {
            fprintf(stderr, "Failed to write to the device: %s\n", strerror(errno));
        } else {
            printf("Command sent to kernel module.\n");
             // Read the response from the device
            memset(read_buf, 0, sizeof(read_buf));
            num_read = read(fd, read_buf, sizeof(read_buf) - 1); // Read data into buffer
            if (num_read > 0) {
                printf("Received from kernel: %s\n", read_buf);
            } else if (num_read < 0) {
                perror("Failed to read from device");
            }
        }
    }

    close(fd);
    printf("Exiting chess game. Goodbye!\n");
    return 0;
}

