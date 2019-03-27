#ifndef ATTR_HH
#define ATTR_HH

// beware utf8!
// http://www.cplusplus.com/reference/codecvt/codecvt_utf8/
// std::u32string
//
class NLATTR
{
};

void decode_nl80211_attr(struct nlattr* tb_msg[], size_t counter);

#endif

