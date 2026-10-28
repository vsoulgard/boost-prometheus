//
// Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_PROMETHEUS_SERIALIZE_HPP
#define BOOST_PROMETHEUS_SERIALIZE_HPP

#include <boost/prometheus/detail/format.hpp>

namespace boost::prometheus {

/** \brief Serialize registry and append the result to a buffer.
 *
 * \tparam FormatType Export format.
 * Supported: `format::prometheus_0_0_4`, `format::openmetrics_1_0`.
 * \tparam AppendEOF If `true` (default), append `"# EOF\n"` for
 * formats that define it (e.g. OpenMetrics 1.0);
 * ignored by formats without an EOF marker.
 * \tparam BufferType Output buffer.
 * Must provide `size()`, `capacity()`, `data()`, 
 * `reserve()` and `resize()` methods.
 * Both `std::string` and `std::vector<char>` satisfy this.
 * \param registry Metric registry to be serialized.
 * \param out_buffer Buffer to append the serialized data to.
 * Existing contents are preserved.
 * \throws std::bad_alloc if the buffer needs to grow.
 */
template <typename FormatType, bool AppendEOF = true, typename RegistryType, typename BufferType>
void serialize(RegistryType& registry, BufferType& out_buffer)
{
    format::serialize<FormatType, RegistryType, BufferType, AppendEOF>(registry, out_buffer);
}

/** \brief Serialize static registry and append the result to a buffer.
 *
 * \tparam FormatType Export format.
 * Current supported format types are `format::prometheus_0_0_4`
 * and `format::openmetrics_1_0`.
 * \tparam StaticRegistryType Type of static registry to
 * be serialized.
 * \tparam AppendEOF If `true` (default), append `"# EOF\n"` for
 * formats that define it (e.g. OpenMetrics 1.0);
 * ignored by formats without an EOF marker.
 * \tparam BufferType Output buffer.
 * Must provide `size()`, `capacity()`, `data()`, 
 * `reserve()` and `resize()` methods.
 * Both `std::string` and `std::vector<char>` satisfy this.
 * \param out_buffer Buffer to append the serialized data to. Existing
 * contents are preserved.
 * \throws std::bad_alloc if the buffer needs to grow.
 */
template <typename FormatType, typename StaticRegistryType, bool AppendEOF = true, typename BufferType>
void serialize(BufferType& out_buffer)
{
    format::serialize<FormatType, StaticRegistryType, BufferType, AppendEOF>(out_buffer);
}

} // namespace boost::prometheus

#endif
