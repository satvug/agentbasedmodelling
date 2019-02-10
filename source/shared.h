
#define array_count(array) sizeof(array) / sizeof(array[0])
#define member_offset(_struct, member) &((_struct *)0)->member
#define assert(expr) if(!(expr)) *(u32 *)0 = 0;
#define crash() assert(false)

inline u32 get_bit_offset(u32 value)
{
    u32 result = 0;

    while(((value >> result) & 1) == 0 && result < 32)
    {
        ++result;
    }

    return result;
}

void ptr_swap_(void **a, void **b)
{
	void *c = *a;
	*a = *b;
	*b = c;
}

#define ptr_swap(a, b) ptr_swap_((void **)(a), (void **)(b))

struct File
{
    void *data;
    u64 size;
};

//
// String stuff
//

struct String
{
    char *data;
    u32 length;
};

// NOTE: excluding zero-terminator
#define const_string(data) {data, sizeof(data) - 1}

inline u32 string_length(char *data)
{
    u32 result = 0;
    while(*data++)
    {
        ++result;
    }
    return result;
}

inline String string(char *data)
{
    String result = {data, string_length(data)};
    return result;
}

inline String string(File file)
{
	String result = {(char *)file.data, (u32)file.size};
	return result;
}

bool string_contains(String a, String b)
{
    bool result = false;

    for(u32 i = 0; i <= a.length - b.length && !result;)
    {
        u32 j = 0;

        while(j < b.length && a.data[i + j] == b.data[j])
        {
            ++j;
        }

        result = j == b.length;
        i += j + 1;
    }
    
    return result;
}

inline bool string_contains(String a, char *b)
{
    bool result = string_contains(a, string(b));
    
    return result;
}

inline bool string_contains(char *a, String b)
{  
    bool result = string_contains(string(a), b);
    
    return result;
}

inline bool string_contains(char *a, char *b)
{
    bool result = string_contains(string(a), string(b));
    
    return result;
}

inline bool equal(String a, String b)
{
	bool result = a.length == b.length;
	for(u32 i = 0; i < a.length && result; ++i)
	{
		result = a.data[i] == b.data[i];
	}
	return result;
}

inline bool equal(String a, char *b)
{
	bool result = equal(a, string(b));
	return result;
}

inline bool is_whitespace(char c)
{
	bool result = (c == ' ' || c == '\n' || c == '\r' || c == '\t');
	return result;
}

inline bool is_number(char c)
{
	bool result = (c >= '0' && c <= '9') || c == '.' || c == '-';
	return result;
}

String eat_word(char *string)
{
	String result = {string, 0};
	while(result.data[result.length] && !is_whitespace(result.data[result.length]))
	{
		++result.length;
	}
	return result;
}

String eat_word(String string)
{
	String result = {string.data, 0};
	for(u32 i = 0; i < string.length && !is_whitespace(string.data[i]); ++i)
	{
		++result.length;
	}
	return result;
}

inline bool is_alphabetic(char c)
{
	bool result = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
	return result;
}

char *eat_whitespace(char *string)
{
    char *result = string;
	while(*result && is_whitespace(*result))
	{
		++result;
	}
	return result;
}

String eat_whitespace(String string)
{
	String result = string;
	while(result.length && is_whitespace(result.data[0]))
	{
		++result.data;
		--result.length;
	}
	return result;
}

inline String advance(String string, s32 advancement)
{
	if(advancement > (s32)string.length)
	{
		advancement = string.length;
	}
	String result;
    result.data = string.data + advancement;
    result.length = string.length - advancement;
	return result;
}

bool string_to_integer(String string, s32 *result_ptr)
{
	bool success = true;
	bool negative = string.data[0] == '-';
	s32 result = 0;
	for(u32 i = negative ? 1 : 0; i < string.length && success; ++i)
	{
		char c = string.data[i];
		if(c >= '0' && c <= '9')
		{
			result = 10 * result + (c - '0');
		}
		else
		{
			success = false;
		}
	}
	if(success)
	{
		if(negative)
		{
			result = -result;
		}
		*result_ptr = result;
	}
	return success;
}

bool string_to_float(String string, f32 *result_ptr)
{
	bool success = true;
	bool negative = string.data[0] == '-';
	f32 current_fractional_multiplier = 1.0f;
	f32 result = 0.0f;
	for(u32 i = negative ? 1 : 0; i < string.length && success; ++i)
	{
		char c = string.data[i];
		if(current_fractional_multiplier == 1.0f)
		{
			if(c >= '0' && c <= '9')
			{
				result = 10.0f * result + (f32)(c - '0');
			}
			else if(c == '.')
			{
				current_fractional_multiplier = 0.1f;
			}
			else
			{
				success = false;
			}
		}
		else
		{
			if(c >= '0' && c <= '9')
			{
				result = result + current_fractional_multiplier * (c - '0');
				current_fractional_multiplier *= 0.1f;
			}
			else
			{
				success = false;
			}
		}
	}
	if(success)
	{
		if(negative)
		{
			result = -result;
		}
		*result_ptr = result;
	}
	return success;
}

// yeah
inline void append(String *a, char *appendix, u32 max_length)
{
	char *cursor = appendix;
	while(*cursor && a->length < max_length)
	{
		a->data[a->length++] = *cursor++;
	}
}

inline void append(String *a, char appendix, u32 max_length)
{
    if(a->length < max_length)
	{
		a->data[a->length++] = appendix;
	}
}

inline void append(String *a, String appendix, u32 max_length)
{
	u32 length = min(appendix.length, max_length - a->length);
	for(u32 i = 0; i < length; ++i)
	{
		a->data[a->length++] = appendix.data[i];
	}
}

inline void append(String *a, u32 appendix, u32 max_length)
{
	u32 char_count = 0;
	char tmp[64];
	do
	{
		tmp[char_count++] = '0' + appendix % 10;
		appendix /= 10;
	}
	while(appendix);
	
	do
	{
		a->data[a->length++] = tmp[--char_count];
	}
	while((a->length - 1 < max_length) && (char_count));
}

inline void append(String *a, s32 appendix, u32 max_length)
{
	if(appendix < 0)
	{
		a->data[a->length++] = '-';
		appendix = -appendix;
	}

	append(a, (u32)appendix, max_length);
}

void append(String *a, f32 appendix, u32 max_length, u32 fractional_part_length = 6)
{
	if(appendix < 0)
	{
		a->data[a->length++] = '-';
		appendix = -appendix;
	}
	f64 appendix64 = (f64)appendix;

	u32 char_count = 0;
	char tmp[64];

	u32 appendix_integer = (u32)appendix64;
	f64 fractional_part = appendix64 - (f64)appendix_integer;
	for(u32 i = 0; i < fractional_part_length && fractional_part > 0.0; ++i)
	{
		fractional_part *= 10.0;
	}

	u32 fractional_part_integer = (u32)fractional_part;
	if((fractional_part - (f64)fractional_part_integer) > 0.5)
	{
		++fractional_part_integer;
	}
	
	while(fractional_part_length--)
	{
		tmp[char_count++] = '0' + fractional_part_integer % 10;
		fractional_part_integer /= 10;
	}
	tmp[char_count++] = '.';

	do
	{
		tmp[char_count++] = '0' + appendix_integer % 10;
		appendix_integer /= 10;
	}
	while(appendix_integer);
	
	do
	{
		a->data[a->length++] = tmp[--char_count];
	}
	while((a->length - 1 < max_length) && (char_count));
}

// NOTE: only the minimal functionality
void append_formatted(String *dst, u32 max_length, char *format, ...)
{
	va_list arguments;
	va_start(arguments, format);

	for(char *cursor = format; *cursor && dst->length < max_length; ++cursor)
	{
		if(cursor[0] == '%')
		{
			switch(cursor[1])
			{
				case 'c':
				{
					char c = va_arg(arguments, char);
					append(dst, c, max_length);
					break;
				}
				case 's':
				{
					char *string = va_arg(arguments, char *);
					append(dst, string, max_length);
					break;
				}
				case 'd':
				{
					s32 value = va_arg(arguments, s32);
					append(dst, value, max_length);
					break;
				}
				case 'u':
				{
					u32 value = va_arg(arguments, u32);
					append(dst, value, max_length);
					break;
				}
				case 'f':
				{
					//f32 value = va_arg(arguments, f32);
					f32 value = (f32)va_arg(arguments, f64);
					append(dst, value, max_length);
					break;
				}
				default:
				{
					dst->data[dst->length++] = cursor[1];
					break;
				}
			}
			++cursor;
		}
		else
		{
			dst->data[dst->length++] = cursor[0];
		}
	}
	
	va_end(arguments);
}

void append(String *a, V2 appendix, u32 max_length, char *separator = " ")
{
	append(a, appendix.x, max_length);
	append(a, separator, max_length);
	append(a, appendix.y, max_length);
}

/*
// TODO ? more printf'y version of that
void append_line(String *a, char *prefix, f32 value, u32 max_length)
{
	append(a, prefix, max_length);
	append(a, value, max_length);
	append(a, '\n', max_length);
}
*/
