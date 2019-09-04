/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2019 Mark Callow.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @internal
 * @file vk2dfd.c
 *
 * @author Mark Callow, Edgewise Consulting.
 */

#include "dfd.h"

/**
 * @internal
 * @~English
 * @brief Create a DFD matching a VkFormat.
 *
 * @param[in] format    VkFormat for which to create a DFD.
 *
 * @return      pointer to the created DFD or 0 if format not supported or
 *              unrecognized. Caller is responsible for freeing the created
 *              DFD.
 */
uint32_t*
vk2dfd(enum VkFormat format)
 {
     switch (format) {
#include "vk2dfd.inl"
         default: return 0;
     }
 }

