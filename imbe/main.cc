#include <cstdio>
#include <cstdlib>
#include <imbe.h>

int
main(int ac, char **av)
{
	op25_imbe *imbe = new op25_imbe();
   while(--ac) {
      FILE *fp = fopen(*++av, "r");
      if(fp) {
         uint8_t buf[18];
         while(1 == fread(buf, sizeof(buf), 1, fp)) {
            imbe->decode(buf);
         }
         fclose(fp);
      } else {
         perror(*av);
      }
   }
   return(0);
}
