/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/


#include "mdl_helper.h"

std::string Mdl_helper::qualified_to_simple_name(const std::string& qualified_name)
{
    if (qualified_name.empty()) return "";      // depending on the "encoding of the root node"
    if (qualified_name == "::") return "::";

    const size_t pos_open_parenthesis = qualified_name.find_first_of('(');
    const size_t pos_simple_name = qualified_name.substr(0, pos_open_parenthesis).rfind("::") + 2;
    return qualified_name.substr(pos_simple_name);
}
