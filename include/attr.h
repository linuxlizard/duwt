#ifndef ATTR_HH
#define ATTR_HH

class NLATTR
{
};

void decode_nl80211_attr(struct nlattr* tb_msg[], size_t counter);

#endif

