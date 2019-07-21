#  Load an OUI file containing the 3-byte Organizationally Unique Identifiers
#

_oui_dict = None

def load_oui():
    # sanity check:
    # grep "^[0-9A-F][0-9A-F]-" /usr/share/hwdata/oui.txt | wc

    # Fedora
    # provided by package hwdata (Fedora 30)
    #
    # Ubuntu
    # hwdata package in Ubuntu doesn't contain oui.txt :rage:
    # ieee-data package does contain /usr/share/ieee-data/oui.txt which is the
    # same format
    #
    oui_files = ("/usr/share/hwdata/oui.txt", "/usr/share/ieee-data/oui.txt")

    def load_file(infilename):
        with open(infilename) as infile:
            for line in infile.readlines():
                try:
                    if line[2] == '-' and line[5] == '-':
                        yield line[0:8], line[18:].rstrip()
                except IndexError:
                    continue

    for infilename in oui_files:
        try:
            yield from load_file(infilename)
            break
        except FileNotFoundError:
            pass

def vendor_lookup(oui):
    global _oui_dict
    if _oui_dict is None:
        # first try hwdata package
        _oui_dict = dict(load_oui())

    # TODO any other packages that might contain an OUI file?
    # Someone has to have solved this before.

    return _oui_dict[oui]
