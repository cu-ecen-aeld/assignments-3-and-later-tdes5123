#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    // Open syslog for logging
    openlog("writer", LOG_PID | LOG_CONS, LOG_USER);

    // Check that the two arguments exist
    if(argc < 2) {
        printf("Missing arguments\n");
        syslog(LOG_ERR, "Missing arguments");
        return 1;
    }

    char* writefile = argv[1];
    char* writestr = argv[2]; 

    // Open the file for writing
    FILE* file = fopen(writefile, "w");
    if (file == NULL) {
        printf("Failed to open file");
        syslog(LOG_ERR, "Failed to open file: %s", writefile);
        return 1;
    }

    syslog(LOG_INFO, "Writing %s to %s", writestr, writefile);

    // Write the string to the file
    fprintf(file, "%s", writestr);
    fclose(file);
    return 0;
}