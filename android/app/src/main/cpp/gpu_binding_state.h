#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

class PGAndroidGpuBindingState {
public:
    struct ConstantBuffer { uint32_t objectId=0, offset=0, size=0; };
    struct UavBinding { uint32_t objectId=0, counterObjectId=0xffffffffu; };
    struct RenderTarget { uint32_t viewId=0, mip=0, slice=0, flags=0; };
    struct DepthTarget { uint32_t viewId=0, mip=0, slice=0, flags=0, readOnly=0; };

    bool setConstantBuffers(const uint32_t *p, uint32_t n) {
        if (!p || n < 3 || p[0] >= 3 || p[1] > 16 || p[2] > 16-p[1] || n < 3u+3u*p[2]) return false;
        for (uint32_t i=0;i<p[2];++i) cb_[p[0]][p[1]+i]={p[3+i*3],p[4+i*3],p[5+i*3]};
        return true;
    }
    bool setShaderResources(const uint32_t *p, uint32_t n) {
        if (!p || n < 3 || p[0] >= 3 || p[1] > 128 || p[2] > 128-p[1] || n < 3u+p[2]) return false;
        for (uint32_t i=0;i<p[2];++i) srv_[p[0]][p[1]+i]=p[3+i];
        return true;
    }
    bool setSamplers(const uint32_t *p, uint32_t n) {
        if (!p || n < 3 || p[0] >= 3 || p[1] > 16 || p[2] > 16-p[1] || n < 3u+p[2]) return false;
        for (uint32_t i=0;i<p[2];++i) sampler_[p[0]][p[1]+i]=p[3+i];
        return true;
    }
    bool setUnorderedAccess(const uint32_t *p, uint32_t n) {
        if (!p || n < 3 || p[0] >= 3 || p[1] > 8 || p[2] > 8-p[1] || n < 3u+2u*p[2]) return false;
        for (uint32_t i=0;i<p[2];++i)
            uav_[p[0]][p[1]+i]={p[3+i],p[3+p[2]+i]};
        return true;
    }
    void clear() {
        cb_={}; srv_={}; sampler_={}; uav_={};
    }
    const ConstantBuffer &constantBuffer(uint32_t stage,uint32_t slot) const { return cb_[stage][slot]; }
    uint32_t shaderResource(uint32_t stage,uint32_t slot) const { return srv_[stage][slot]; }
    uint32_t sampler(uint32_t stage,uint32_t slot) const { return sampler_[stage][slot]; }
    const UavBinding &unorderedAccess(uint32_t stage,uint32_t slot) const { return uav_[stage][slot]; }
    const RenderTarget &renderTarget(uint32_t slot) const { return rt_[slot]; }
    const DepthTarget &depthTarget() const { return ds_; }
private:
    std::array<std::array<ConstantBuffer,16>,3> cb_{};
    std::array<std::array<uint32_t,128>,3> srv_{};
    std::array<std::array<uint32_t,16>,3> sampler_{};
    std::array<std::array<UavBinding,8>,3> uav_{};
    std::array<RenderTarget,8> rt_{};
    DepthTarget ds_{};
};
