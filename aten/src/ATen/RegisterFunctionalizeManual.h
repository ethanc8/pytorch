#pragma once

#include <ATen/EmptyTensor.h>
#include <ATen/FunctionalInverses.h>
#include <ATen/FunctionalTensorWrapper.h>
#include <ATen/MemoryOverlap.h>
#include <ATen/core/LegacyTypeDispatch.h>

#include <ATen/ops/empty_strided_native.h>

namespace at {
namespace functionalization {

// This keyset is used by functionalization when it calls into meta kernels
// to accurately propagate stride metadata.
// Exclude any modes: the purpose of calling into meta kernels is only as an
// implementation detail to perform shape inference, and we don't want any modal
// keys to run. Specifically, we want to prevent functionalization and Python
// modes from running.
constexpr auto exclude_keys_for_meta_dispatch = c10::functorch_transforms_ks |
    c10::DispatchKeySet({
        c10::DispatchKey::FuncTorchDynamicLayerBackMode,
        c10::DispatchKey::FuncTorchDynamicLayerFrontMode,
        c10::DispatchKey::Python,
        c10::DispatchKey::PreDispatch,

    });

// Helper around at::has_internal_overlap.
// The ATen util is used in hot-path eager mode: it's always fast,
// but might return TOO_HARD sometimes.
// During functionalization, we're ok taking a bit longer
// to detect memory overlap.
inline bool has_internal_overlap_helper(const at::Tensor t) {
  auto has_overlap = at::has_internal_overlap(t);
  if (has_overlap == at::MemOverlap::Yes)
    return true;
  if (has_overlap == at::MemOverlap::No)
    return false;
  return false;
}

inline Tensor to_meta(const Tensor& t) {
  if (!t.defined())
    return t;
  return at::native::empty_strided_meta_symint(
      t.sym_sizes(),
      t.sym_strides(),
      /*dtype=*/c10::make_optional(t.scalar_type()),
      /*layout=*/c10::make_optional(t.layout()),
      /*device=*/c10::make_optional(c10::Device(kMeta)),
      /*pin_memory=*/c10::nullopt);
}

inline c10::optional<Tensor> to_meta(const c10::optional<Tensor>& t) {
  if (t.has_value()) {
    return c10::make_optional<Tensor>(to_meta(*t));
  }
  return c10::nullopt;
}

inline std::vector<Tensor> to_meta(at::ITensorListRef t_list) {
  std::vector<Tensor> outputs;
  outputs.reserve(t_list.size());
  for (const auto& tensor : t_list) {
    outputs.push_back(to_meta(tensor));
  }
  return outputs;
}

inline c10::List<Tensor> to_meta(const c10::List<Tensor>& t_list) {
  c10::List<Tensor> outputs;
  outputs.reserve(t_list.size());
  for (const auto i : c10::irange(t_list.size())) {
    outputs.push_back(to_meta(t_list[i]));
  }
  return outputs;
}

inline c10::List<c10::optional<Tensor>> to_meta(
    const c10::List<c10::optional<Tensor>>& t_list) {
  c10::List<c10::optional<Tensor>> outputs;
  outputs.reserve(t_list.size());
  for (const auto i : c10::irange(t_list.size())) {
    outputs.push_back(to_meta(t_list[i]));
  }
  return outputs;
}

} // namespace functionalization
} // namespace at
