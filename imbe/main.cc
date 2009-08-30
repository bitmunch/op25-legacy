#include <cstdio>
#include <cstdlib>
#include <imbe.h>

#include <boost/scoped_ptr.hpp>

using namespace boost;

int
main(int ac, char **av)
{
	scoped_ptr<software_imbe_decoder> imbe(new software_imbe_decoder());
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
