#include <stdio.h>

#include "date.h"

/* Howard Hinnant's days_from_civil, backwards. */
void formatDate( unsigned long unixTime, char out[11] ) {
  long z = (long)( unixTime / 86400UL ) + 719468L;
  long era = z / 146097L;
  long doe = z - era * 146097L;
  long yoe = ( doe - doe / 1460 + doe / 36524L - doe / 146096L ) / 365;
  long doy = doe - ( 365 * yoe + yoe / 4 - yoe / 100 );
  long mp = ( 5 * doy + 2 ) / 153;
  int day = (int)( doy - ( 153 * mp + 2 ) / 5 + 1 );
  int month = (int)( mp < 10 ? mp + 3 : mp - 9 );
  long year = yoe + era * 400 + ( month <= 2 ? 1 : 0 );
  snprintf( out, 11, "%04ld-%02d-%02d", year, month, day );
}
