#include <string>
#include <ctype.h>

struct ci_char_traits : public std::char_traits<char>
{
  static bool eq( char c1, char c2 ) {
    return ::tolower(c1) == ::tolower(c2);
  }

  static bool ne( char c1, char c2 ) {
    return ::tolower(c1) != ::tolower(c2);
  }

  static bool lt( char c1, char c2 ) {
    return ::tolower(c1) < ::tolower(c2);
  }

  static int compare( const char* s1,
                      const char* s2,
                      size_t n ) {
    return ::strncasecmp( s1, s2, n );
  }

  static const char*
  find( const char* s, int n, char a ) {
    while ( n-- > 0 && ::tolower(*s) != ::tolower(a) ) ++s;
    return s;
  }
};

typedef std::basic_string<char, ci_char_traits> ci_string;
