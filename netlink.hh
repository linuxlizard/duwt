#ifndef NETLINK_HPP
#define NETLINK_HPP

class Netlink
{
public:
	// https://en.cppreference.com/w/cpp/language/rule_of_three
	// https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#S-ctor
	Netlink();
	virtual ~Netlink();  // Destructor

	// definitely don't want copy constructors
	Netlink(const Netlink&) = delete; // Copy constructor
	Netlink& operator=(const Netlink&) = delete;  // Copy assignment operator

	// added move constructors to learn how to use move constructors
	Netlink(Netlink&&);  // Move constructor
	Netlink& operator=(Netlink&&); // Move assignment operator

	int get_scan_results(void);

private:
	struct nl_sock* nl_sock;
	int nl80211_id;
	struct nl_cb* s_cb;

};

class Cfg80211
{

};

#endif
