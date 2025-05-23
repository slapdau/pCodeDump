/*
   Copyright 2017-2024 Craig McGeachie

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

#include <boost/test/unit_test.hpp>

#include <iostream>
#include <cstdint>
#include <sstream>
#include "../pcodedump/textio.hpp"

    BOOST_AUTO_TEST_CASE(hexdump_array)
    {
        std::wstring const expected = L""
            "-> 0000: 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f    ................\n"
            "-> 0010: 10 11 12 13                                        ....\n";
        std::uint8_t const data[] = {
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
            0x10, 0x11, 0x12, 0x13,
        };

        std::wostringstream out;
        pcodedump::hexdump(out, L"-> ", std::begin(data), std::end(data));
        bool same = out.str() == expected;
        BOOST_TEST_CHECK(same);
    }
