#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main(int count,char** args){
 const char* state=getenv("BEZELBREW_KDE_FIXTURE_STATE");char mode[32]="single";if(state){FILE* f=fopen(state,"r");if(f){fgets(mode,sizeof(mode),f);fclose(f);}}
 if(count==3&&!strcmp(args[1],"--list-available")&&!strcmp(args[2],"--id-only")){puts("test-phone-123");return 0;}
 if(count==3&&!strcmp(args[1],"--list-available")&&!strcmp(args[2],"--id-name-only")){
  Sleep(250);if(!strncmp(mode,"none",4)){fputs("No devices found\n",stderr);return 0;}if(!strncmp(mode,"fail",4)){fputs("Daemon not reachable\n",stderr);return 1;}
  puts("test-phone-123 Personal test phone");if(!strncmp(mode,"many",4))puts("test-phone-456 Spare test phone");return 0;
 }
 if(count==4&&!strcmp(args[1],"--device")&&!strcmp(args[3],"--list-notifications")){
  if(!strcmp(args[2],"test-phone-123")){puts("- Test Mail: Connector integration fixture\n- Test Chat: Second fixture notification");return 0;}
  if(!strcmp(args[2],"test-phone-456")){puts("- Test Messages: Notification from the selected second phone");return 0;}
 }
 return 1;
}
