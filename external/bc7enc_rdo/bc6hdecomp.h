#pragma once

namespace bc6hdecomp 
{
extern "C" void
bcdec_bc6h_half(const void* compressedBlock, void* decompressedBlock, int destinationPitch, int isSigned);
}  // namespace bc6hdecomp
