/*
 * Released: {{DATE}}
 * Authors: Benjamin Kim, William Klock, Boon Pang Lim, Siyan Lin, Helen Zhang
 * Copyright Novumind Inc. 2018, all rights reserved.
 */

/**
 * The novu namespace contains the Application Programming Interface for
 * Novumind's NovuTensor series of products. The SDK supports the Black Bear
 * FPGA (bb) and Grizzly-ASIC (gz) product lines. This SDK is part of the
 * NovuRT platform.
 */

#ifndef __NOVU_NET_H__
#define __NOVU_NET_H__

#include <memory>
#include <string>
#include "buffer.h"

namespace novu {

/// @brief Represents a neural network accelerated by NovuTensor.
class NNet {
   protected:
    NNet();

   public:
    /// @brief Return input feature map dimensions.
    /// @param n Batch size.
    /// @param c Channels.
    /// @param h Height.
    /// @param w Width.
    virtual bool get_ifm_dims(size_t& n, size_t& c, size_t& h,
                              size_t& w) const = 0;

    /// @brief Return output feature map dimensions.
    /// @param n Batch size.
    /// @param c Channels.
    /// @param h Height.
    /// @param w Width.
    virtual bool get_ofm_dims(size_t& n, size_t& c, size_t& h,
                              size_t& w) const = 0;

    /// @name SetInput methods.
    /// @{

    /// @brief Sends an input feature map for processing.
    /// @details This sends an input feature map to the runtime to be queued for
    /// processing. This version uses a floating-point feature map, which is
    /// then scaled and quantized during processing. SetInput must be paired
    /// with a subsequent GetOutput processing.
    /// @param input_buffer_ptr Pointer to the floating point Buffer which holds
    /// the data.
    virtual void SetInput(std::shared_ptr<Buffer<float>> input_buffer_ptr) = 0;

    /// @brief Sends an 8-bit signed integer feature map for processing.
    /// @details Sends a 8-bit signed integer buffer for quantization and
    /// subsequent processing. In this version, an already scaled and quantized
    /// feature map containing signed 8-bit integers is used. A SetInput must be
    /// paired with a subsequent GetOutput
    /// @param input_buffer_ptr Pointer to the 8-bit signed integer Buffer which
    /// holds the data.
    virtual void SetInput(std::shared_ptr<Buffer<int8_t>> input_buffer_ptr) = 0;
    //
    /// @}

    /// @brief Receives processed data from the novutensor device.
    /// @details Retrieves output feature map via a shared_ptr to a Buffer.
    /// The Buffer object to which the output_buffer_ptr is created within
    /// GetOutput, so there is no need to pre-allocate Buffer before using
    /// GetOutput. Example usage:
    ///     std::shared_ptr<Buffer> p;
    ///     nptr->GetOutput(p);
    /// @param output_buffer_ptr Pointer to the Buffer which holds the
    /// formatted data for the output feature map.
    virtual void GetOutput(
        std::shared_ptr<Buffer<float>>& output_buffer_ptr) = 0;

    /// @brief Prints timings for operations involved in running network.
    /// @details This prints timing information for each hardware/software
    /// subsections of the neural network in milliseconds. Software subsections
    /// print inference time. Inference time is the latency of the forward
    /// computation of software subsection of the neural network. Hardware
    /// subsections print unpack time, IFM transfer time, NovuTensor execution
    /// time, OFM transfer time, and pack time. The unpack and pack time
    /// measures the latency of the pre-processing and post-processing times of
    /// the feature maps before being transfered to and received from the
    /// NovuTensor device, respectively. IFM transfer and OFM transfer time
    /// refer to the latencies of transferring to and receiving the feature maps
    /// from the NovuTensor device, respectively. Execution time refers to the
    /// latency of performing inference on the NovuTensor device.
    /// @param o Target stream to which to print timing information.
    virtual void PrintTimingInfo(std::ostream& o) const = 0;

    /// @brief NNet destructor
    virtual ~NNet();
};

/// @brief Creates NovuTensor accelerated neural network object.
/// @details This creates an object to manage the running of NovuTensor
/// accelerated neural networks. The object initializes the NovuTensor device,
/// loading associated parameters to the hardware memory and prepares it for
/// inference. A runtime exception is thrown if the network cannot be
/// initialized. This function will automatically detect the type of model we
/// have, either pure hardware or hybrid, and perform the required
/// initialization and loading.
/// @param model A file path to a model, which can be a hw model or a
/// full model.
/// @param num_devices Number of NovuTensor devices to accelerate neural
/// network model (default to 1 NovuTensor device).
/// @return A handle to an initialized neural network object.
NNet* CreateNNet(const char* model, size_t num_devices = 1);

/// @brief Returns a string representing the current SDK version.
const std::string GetVersionString();

/// @brief Returns a unique integer identifier representing SDK version.
uint64_t GetVersionId();

}  // namespace novu

#endif
