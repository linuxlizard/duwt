#include <string.h>

#include "core.h"
#include "ie.h"
#include "ie_he.h"

int ie_he_capabilities_new(struct IE* ie)
{
	INFO("%s\n", __func__);
	CONSTRUCT(struct IE_HE_Capabilities)

	return 0;
}

void ie_he_capabilities_free(struct IE* ie)
{
	INFO("%s\n", __func__);
	DESTRUCT(struct IE_HE_Capabilities)
}

int ie_he_operation_new(struct IE* ie)
{
	INFO("%s\n", __func__);
	CONSTRUCT(struct IE_HE_Operation)

	return 0;
}

void ie_he_operation_free(struct IE* ie)
{
	INFO("%s\n", __func__);
	DESTRUCT(struct IE_HE_Operation);
}

