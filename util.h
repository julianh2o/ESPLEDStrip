void *memchr(const void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char*)s;
    while( n-- )
        if( *p != (unsigned char)c )
            p++;
        else
            return p;
    return 0;
}

const char* stristr(const char* str1, const char* str2 ) {
    const char* p1 = str1 ;
    const char* p2 = str2 ;
    const char* r = *p2 == 0 ? str1 : 0 ;

    while( *p1 != 0 && *p2 != 0 )
    {
        if( tolower( *p1 ) == tolower( *p2 ) )
        {
            if( r == 0 )
            {
                r = p1 ;
            }

            p2++ ;
        }
        else
        {
            p2 = str2 ;
            if( tolower( *p1 ) == tolower( *p2 ) )
            {
                r = p1 ;
                p2++ ;
            }
            else
            {
                r = 0 ;
            }
        }

        p1++ ;
    }

    return *p2 == 0 ? r : 0 ;
}

unsigned long lastStart;
void start() {
  lastStart = millis();
}

void stop(String s) {
  Serial.print(s);
  Serial.print(": ");
  Serial.print(millis()-lastStart);
  Serial.print("ms");
  Serial.println();
}

void debugHex(const char *buf, int len) {
    for (int i=0; i<len; i++) {
        Serial.print(" ");
        if (buf[i] >= 32 && buf[i] <= 126) {
            Serial.print(buf[i]);
        } else {
            Serial.print(" ");
        }
        Serial.print(" ");
    }
    Serial.println();

    for (int i=0; i<len; i++) {
        if (buf[i] < 16) Serial.print("0");
        Serial.print(buf[i],HEX);
        Serial.print(" ");
    }
    Serial.println();
}
