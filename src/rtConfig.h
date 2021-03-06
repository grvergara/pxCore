/*

 pxCore Copyright 2005-2017 John Robinson

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

// rtConfig.h

#ifndef RT_CONFIG_H
#define RT_CONFIG_H

#if defined(RT_PLATFORM_LINUX)
#elif defined(RT_PLATFORM_WINDOWS)
#else
#error "Must define be RT_PLATFORM_LINUX"
#endif

#endif
