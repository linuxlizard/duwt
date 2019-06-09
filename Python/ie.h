#ifndef IE_H
#define IE_H

const char* ie_get_name(uint8_t id);

int ie_decode(uint8_t id, uint8_t len, uint8_t* buf, PyObject* dest_dict);

#endif

