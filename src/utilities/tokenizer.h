/* 
 * Copyright © 2025, UChicago Argonne, LLC
 * All Rights Reserved
 * Software Name: Remote Test Harness
 * By: Argonne National Laboratory
 * 
 * GNU GENERAL PUBLIC LICENSE
 * Version 3, 29 June 2007
 * Copyright © 2007 Free Software Foundation, Inc. <https://fsf.org/>
 * Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed.
 * 
 * See the LICENSE file for the full license text.
 */


//=========================================================================================================
// tokenizer.h - Defines A class that tokenizes strings
//=========================================================================================================
#pragma once
#include <string>
#include <vector>


class CTokenizer
{
public:
    std::vector<std::string> parse(const std::string& input);
};