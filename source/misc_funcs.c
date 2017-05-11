#include "misc_funcs.h"

extern inline u8 isEmptyString(const char* buffer);
extern inline double ceiling(double value);

size_t dtoa(double value, char* buffer, u8 precision)
{
  //adapted from https://github.com/client9/stringencoders
  //makes a few assumptions:
  //  1. No nans
  //  2. Number is positive and small, so don't worry about overflow
  //  3. Precision is positive and small, typically 2
  
  static const double powers_of_10[] = { 1, 10, 100, 1000, 10000, 100000, 1000000,
    10000000, 100000000, 1000000000 };
  
  double diff = 0.0;
  char* wstr = buffer;

  // given 0.05, precision=1
  // whole = 0
  // tmp = (0.05)* 10 = 0.5
  // frac = 0
  // diff = tmp -frac == 0.5 - 0.0 = 0.5
  //
  int whole = (int)value;
  double tmp = (value - whole) * powers_of_10[precision];
  uint32_t frac = (uint32_t)(tmp);
  diff = tmp - frac;

  if (diff > 0.5)
  {
    ++frac;
    /* handle rollover, e.g.  case 0.99 with precision 1 is 1.0  */
    if (frac >= powers_of_10[precision])
    {
      frac = 0;
      ++whole;
    }
  }
  else if (diff == 0.5 && precision > 0 && (frac & 1))
  {
    /* if halfway, round up if odd, OR
   if last digit is 0.  That last part is strange */
    ++frac;
    if (frac >= powers_of_10[precision])
    {
      frac = 0;
      ++whole;
    }
  }
  else if (diff == 0.5 && precision == 0 && (whole & 1))
  {
    ++frac;
    if (frac >= powers_of_10[precision])
    {
      frac = 0;
      ++whole;
    }
  }

  int count = precision;
  while (count > 0)
  {
    --count;
    *wstr++ = (char)(48 + (frac % 10));
    frac /= 10;
  }
  if (frac > 0)
  {
    ++whole;
  }

  /* add decimal */
  if (precision > 0)
  {
    *wstr++ = '.';
  }

  /* do whole part
 * Take care of sign conversion
 * Number is reversed.
 */
  do
  {
    *wstr++ = (char)(48 + (whole % 10));
  }
  while (whole /= 10);
  *wstr = '\0';
  
  //reverse string
  char swap;
  size_t size = (size_t)(wstr - buffer);
  wstr--;
  while (wstr > buffer)
  {
    swap = *wstr;
    *wstr-- = *buffer;
    *buffer++ = swap;
  }
  
  return size;
}