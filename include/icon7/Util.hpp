// Copyright (C) 2023-2024 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_UTIL_HPP
#define ICON7_UTIL_HPP

namespace icon7
{
template <typename... Args>
auto ConvertLambdaToFunctionPtr(void (*fun)(Args...))
{
	return fun;
}

template <typename Fun> auto ConvertLambdaToFunctionPtr(Fun &&fun)
{
	return +fun;
}
} // namespace icon7

#endif
