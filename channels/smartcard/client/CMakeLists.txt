# FreeRDP: A Remote Desktop Protocol Implementation
# FreeRDP cmake build script
#
# Copyright 2012 Marc-Andre Moreau <marcandre.moreau@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

define_channel_client("smartcard")

set(${MODULE_PREFIX}_SRCS smartcard_main.c smartcard_main.h)

set(${MODULE_PREFIX}_LIBS winpr freerdp ${OPENSSL_LIBRARIES})
if(WITH_SMARTCARD_EMULATE)
  list(APPEND ${MODULE_PREFIX}_LIBS ZLIB::ZLIB)
endif()
add_channel_client_library(${MODULE_PREFIX} ${MODULE_NAME} ${CHANNEL_NAME} FALSE "DeviceServiceEntry")
