#ifndef IE_HPP
#define IE_HPP

#include <iostream>

// base class of 802.11 Information Element
class IE
{
	public:
		IE(uint8_t id, uint8_t len, uint8_t *buf); 
		~IE();

//		std::string repr(void);

		// definitely don't want copy constructors
		IE(const IE&) = delete; // Copy constructor
		IE& operator=(const IE&) = delete;  // Copy assignment operator

		// added move constructors to learn how to use move constructors
		// Move constructor
		IE(IE&& src) 
			: id(src.id), len(src.len), buf(src.buf)
		{
			src.id = -1;
			src.len = -1;
			src.buf = nullptr;
		};

		IE& operator=(IE&& src) // Move assignment operator
		{
			id = src.id;
			len = src.len;
			buf = src.buf;

			src.id = -1;
			src.len = -1;
			src.buf = nullptr;

			return *this;
		};


		friend std::ostream & operator << (std::ostream &, const IE&);

	private:
		uint8_t id;
		uint8_t len;
		uint8_t *buf;

		/* quick notes while I'm thinking of it: SSID could be utf8 
		 * https://www.gnu.org/software/libc/manual/html_node/Extended-Char-Intro.html
		 *
		 * https://stackoverflow.com/questions/55641/unicode-processing-in-c
		 * http://site.icu-project.org/ 
		 * dnf info libicu
		 * dnf info libicu-devel
		 *
		 * https://withblue.ink/2019/03/11/why-you-need-to-normalize-unicode-strings.html
		 */
};

std::ostream& operator<<(std::ostream& os, const IE& ie);

#endif
