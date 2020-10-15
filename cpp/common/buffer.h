/*
 * Released: {{DATE}}
 * Authors: Benjamin Kim, William Klock, Boon Pang Lim, Siyan Lin, Helen Zhang
 * Copyright Novumind Inc. 2018, all rights reserved.
 */
#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <cassert>
#include <cstdlib>

namespace novu {

/// @brief Implements a buffer that holds floating point tensors.
/// @details This implements a simple buffer class, which holds a batch of
/// n feature maps. It is the main data structure used to interact with the API.
/// Internally, the feature map data is in 'C' order, and the data is stored
/// according to the batch, channel, height and width dimensions.
template <typename DataType>
class Buffer {
   public:
    /// @brief Constructs an unallocated buffer to hold feature maps.
    Buffer();

    /// @brief Buffer constructor.
    /// @details This constructs a new buffer object and allocates a single
    /// contiguous array of floating point values big enough to contain a batch
    /// of features maps of the given size. The allocated memory is owned and
    /// managed by this object. The provided size should be valid (all entries
    /// greater than 0), or an exception may be thrown.
    /// @param n Batch size.
    /// @param c Number of channels.
    /// @param h Height size.
    /// @param w Width size.
    Buffer(size_t n, size_t c, size_t h, size_t w);

    /// @brief Buffer constructor with data intialization.
    /// @details Constructs a buffer around a provided block of memory
    /// containing floating point data. This constructor takes in data to
    /// initialize the buffer with. The data will not be deallocated when the
    /// buffer goes out of scope, because it does not own the data.
    /// @param data Pointer to float data to initialization.
    /// @param n Batch size.
    /// @param c Number of channels.
    /// @param h Height size.
    /// @param w Width size.
    Buffer(DataType* data, size_t n, size_t c, size_t h, size_t w);

    /// @brief Buffer destructor.
    ~Buffer();

    /// @brief Copy constructor is disallowed.
    Buffer(const Buffer& b) = delete;

    /// @brief Assignment is disallowed.
    Buffer& operator=(const Buffer& b) = delete;

    /// @brief Move constructor.
    Buffer(const Buffer&& b);

    /// @brief Move assignment operator.
    Buffer& operator=(const Buffer&& b);

    /// @brief Returns the batch size n.
    size_t n() const { return this->n_; }

    /// @brief Returns the channel size c.
    size_t c() const { return this->c_; }

    /// @brief Returns the height h.
    size_t h() const { return this->h_; }

    /// @brief Returns the weight w.
    size_t w() const { return this->w_; }

    /// @brief Zeroes out buffer data.
    void Clear() const;

    /// @brief Returns the size in number of elements of a single feature map.
    /// @details This is 4 times smaller than byteSize, due to floats being 4
    /// bytes long.
    size_t size() const { return this->size_; }

    /// @brief Returns the size in bytes for a single feature map.
    size_t byte_size() const { return this->size_ * sizeof(DataType); }

    /// @brief Returns the full size of the buffer needed to to hold n fms.
    size_t total_size() const { return this->n_ * this->size_; }

    /// @brief Provides direct access to the floating point data.
    DataType* operator*() { return data_; }

    /// @brief Provides direct access to the floating point data.
    /// @details This version provides access for read only buffers.
    const DataType* operator*() const { return data_; }

    /// @brief Provides direct access to each feature map in the buffer.
    /// @details Allows us to use b[0], b[1] , ... to access the 1st, 2nd ...
    /// etc feature map's floating point data.
    DataType* operator[](size_t n) {
        assert(n < this->n_);
        return data_ + n * size_;
    }

    /// @brief Provides direct access to each feature map in the buffer.
    /// @details Allows us to use b[0], b[1] , ... to access the 1st, 2nd ...
    /// etc feature map's floating point data. This version works for const
    /// Buffer.
    const DataType* operator[](size_t n) const {
        assert(n < this->n_);
        return data_ + n * size_;
    }

    /// @brief Allocates a buffer for a given batch size and feature map size.
    /// @details This also reallocates the data area for the feature maps if
    /// they are already allocated.
    /// @param n Batch size.
    /// @param c Number of channels.
    /// @param h Height size.
    /// @param w Width size.
    /// @return True if allocation successful, false otherwise.
    bool Alloc(size_t n, size_t c, size_t h, size_t w);

    /// @brief Deallocates the feature maps in this buffer (if allocated).
    void Dealloc();

   protected:
    /// @details If set to true, this means this object is responsible for
    /// deallocating data_.
    mutable bool owned_;
    /// @details Pointer to contiguous memory location containing floating point
    /// data.
    DataType* data_;
    size_t n_;     ///< Batch size.
    size_t c_;     ///< Channels.
    size_t h_;     ///< Height.
    size_t w_;     ///< Width.
    size_t size_;  ///< Size of single feature map (Channels * Height * Width)
};

}  // namespace novu

#endif
