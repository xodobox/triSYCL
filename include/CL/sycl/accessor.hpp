#ifndef TRISYCL_SYCL_ACCESSOR_HPP
#define TRISYCL_SYCL_ACCESSOR_HPP

/** \file The OpenCL SYCL accessor<>

    Ronan at Keryell point FR

    This file is distributed under the University of Illinois Open Source
    License. See LICENSE.TXT for details.
*/

#include <cstddef>

#include "CL/sycl/access.hpp"
#include "CL/sycl/buffer/detail/accessor.hpp"
#include "CL/sycl/pipe_reservation.hpp"
#include "CL/sycl/pipe/detail/pipe_accessor.hpp"

namespace cl {
namespace sycl {

template <typename T, std::size_t Dimensions, typename Allocator>
struct buffer;
template <typename T>
struct pipe;
class handler;

/** \addtogroup data Data access and storage in SYCL
    @{
*/

/** The accessor abstracts the way buffer or pipe data are accessed
    inside a kernel in a multidimensional variable length array way.

    \todo Implement it for images according so section 3.3.4.5
*/
template <typename DataType,
          std::size_t Dimensions,
          access::mode AccessMode,
          access::target Target = access::global_buffer>
struct accessor : detail::accessor<DataType, Dimensions, AccessMode, Target> {
  /// \todo in the specification: store the dimension for user request
  static constexpr auto dimensionality = Dimensions;
  using value_type = DataType;
  using reference = value_type&;
  using const_reference = const value_type&;

  // Inherit of the constructors to have accessor constructor from detail
  using detail::accessor<DataType, Dimensions, AccessMode, Target>::accessor;

  /** Construct a buffer accessor from a buffer using a command group
      handler object from the command group scope

      Constructor only available for global_buffer or constant_buffer
      target.

      access_target defines the form of access being obtained.

      \todo Add template allocator type in all the accessor
      constructors in the specification or just use a more opaque
      Buffer type?

      \todo fix specification where access mode should be target
      instead
  */
  template <typename Allocator>
  accessor(buffer<DataType, Dimensions, Allocator> &target_buffer,
           handler &command_group_handler)
    : detail::accessor<DataType, Dimensions, AccessMode, Target> {
    *target_buffer.implementation, command_group_handler } {
    static_assert(Target == access::target::global_buffer
                  || Target == access::target::constant_buffer,
                  "access target should be global_buffer or constant_buffer "
                  "when a handler is used");
  }


  /** Construct a buffer accessor from a buffer using a command group
      handler object from the command group scope

      Constructor only available for host_buffer target.

      access_target defines the form of access being obtained.

      \todo add this lacking constructor to specification
  */
  template <typename Allocator>
  accessor(buffer<DataType, Dimensions, Allocator> &target_buffer)
    : detail::accessor<DataType, Dimensions, AccessMode, Target> {
    *target_buffer.implementation } {
    static_assert(Target == access::target::host_buffer,
                  "without a handler, access target should be host_buffer");
  }


  /** Construct a buffer accessor from a buffer given a specific range for
      access permissions and an offset that provides the starting point
      for the access range using a command group handler object from the
      command group scope

      This accessor limits the processing of the buffer to the [offset,
      offset+range[ for every dimension. Any other parts of the buffer
      will be unaffected.

      Constructor only available for access modes global_buffer,
      host_buffer or constant_buffer (see Table 3.25). access_target
      defines the form of access being obtained (see Table 3.26).

      This accessor is recommended for discard-write and discard read
      write access modes, when the unaffected parts of the processing
      should be retained.
  */
  template <typename Allocator>
  accessor(buffer<DataType, Dimensions, Allocator> &target_buffer,
           handler &command_group_handler,
           range<Dimensions> offset,
           range<Dimensions> range) {
    detail::unimplemented();
  }


  /** Construct an accessor of dimensions Dimensions with elements of type
      DataType using the passed range to specify the size in each
      dimension

      It needs as a parameter a command group handler object from the
      command group scope. Constructor only available if AccessMode is
      local, see Table 3.25.
  */
  accessor(range<Dimensions> allocation_size,
           handler &command_group_handler) {
    detail::unimplemented();
  }

};


/** The pipe accessor abstracts the way pipe data are accessed inside
    a kernel

    A specialization for an non-blocking pipe
*/
template <typename DataType,
          access::mode AccessMode>
struct accessor<DataType, 1, AccessMode, access::pipe> :
    detail::pipe_accessor<DataType, AccessMode, access::pipe> {
  using accessor_detail =
    detail::pipe_accessor<DataType, AccessMode, access::pipe>;
  // Inherit of the constructors to have accessor constructor from detail
  using accessor_detail::accessor_detail;

  /** Construct a pipe accessor from a pipe using a command group
      handler object from the command group scope

      access_target defines the form of access being obtained.
  */
  accessor(pipe<DataType> &p, handler &command_group_handler)
    : accessor_detail { *p.implementation, command_group_handler } { }

  /// Make a reservation inside the pipe
  pipe_reservation<accessor> reserve(std::size_t size) const {
    return accessor_detail::reserve(size);
  }


  /// Get the underlying pipe implementation
  auto &get_pipe_detail() {
    return accessor_detail::get_pipe_detail();
  }

};


/** The pipe accessor abstracts the way pipe data are accessed inside
    a kernel

    A specialization for a blocking pipe
*/
template <typename DataType,
          access::mode AccessMode>
struct accessor<DataType, 1, AccessMode, access::blocking_pipe> :
    detail::pipe_accessor<DataType, AccessMode, access::blocking_pipe> {
  using accessor_detail =
    detail::pipe_accessor<DataType, AccessMode, access::blocking_pipe>;
  // Inherit of the constructors to have accessor constructor from detail
  using accessor_detail::accessor_detail;

  /** Construct a pipe accessor from a pipe using a command group
      handler object from the command group scope

      access_target defines the form of access being obtained.
  */
  accessor(pipe<DataType> &p, handler &command_group_handler)
    : accessor_detail { *p.implementation, command_group_handler } { }


  /// Make a reservation inside the pipe
  pipe_reservation<accessor> reserve(std::size_t size) const {
    return accessor_detail::reserve(size);
  }


  /// Get the underlying pipe implementation
  auto &get_pipe_detail() {
    return accessor_detail::get_pipe_detail();
  }

};


/** Top-level function to break circular dependencies on the the types
    to get the pipe implementation */
template <typename Accessor>
static inline auto &get_pipe_detail(Accessor &a) {
  return a.get_pipe_detail();
  }

/// @} End the data Doxygen group

}
}

/*
    # Some Emacs stuff:
    ### Local Variables:
    ### ispell-local-dictionary: "american"
    ### eval: (flyspell-prog-mode)
    ### End:
*/

#endif // TRISYCL_SYCL_ACCESSOR_HPP
