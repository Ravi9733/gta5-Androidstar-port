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
    struct VertexBuffer { uint32_t objectId=0, stride=0, offset=0; };
    struct IndexBuffer { uint32_t objectId=0, format=0, offset=0; };

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
    bool setVertexBuffers(const uint32_t *p, uint32_t n) {
        if (!p || n < 2 || p[0] > 16 || p[1] > 16-p[0] || n < 2u+3u*p[1]) return false;
        for (uint32_t i=0; i<p[1]; ++i)
            vb_[p[0]+i] = {p[2+i*3], p[3+i*3], p[4+i*3]};
        return true;
    }
    bool setIndexBuffer(const uint32_t *p, uint32_t n) {
        if (!p || n < 3) return false;
        ib_ = {p[0], p[1], p[2]};
        return true;
    }
    bool setShader(const uint32_t *p, uint32_t n) {
        if (!p || n < 2 || p[0] >= 3) return false;
        shader_[p[0]] = p[1];
        return true;
    }
    bool setInputLayout(const uint32_t *p, uint32_t n) {
        if (!p || n < 1) return false;
        inputLayout_ = p[0];
        return true;
    }
    bool setTopology(const uint32_t *p, uint32_t n) {
        if (!p || n < 1) return false;
        topology_ = p[0];
        return true;
    }
    bool setRenderTargets(const uint32_t *p, uint32_t n) {
        if (!p || n < 37u) return false;
        for (uint32_t i=0; i<8; ++i)
            rt_[i] = {p[i*4], p[i*4+1], p[i*4+2], p[i*4+3]};
        ds_ = {p[32], p[33], p[34], p[35], p[36]};
        return true;
    }
    void clear() {
        cb_={}; srv_={}; sampler_={}; uav_={}; rt_={}; ds_={}; vb_={}; ib_={}; shader_={}; inputLayout_=0; topology_=0;
    }
    const ConstantBuffer &constantBuffer(uint32_t stage,uint32_t slot) const { return cb_[stage][slot]; }
    uint32_t shaderResource(uint32_t stage,uint32_t slot) const { return srv_[stage][slot]; }
    uint32_t sampler(uint32_t stage,uint32_t slot) const { return sampler_[stage][slot]; }
    const UavBinding &unorderedAccess(uint32_t stage,uint32_t slot) const { return uav_[stage][slot]; }
    const RenderTarget &renderTarget(uint32_t slot) const { return rt_[slot]; }
    const DepthTarget &depthTarget() const { return ds_; }
    const VertexBuffer &vertexBuffer(uint32_t slot) const { return vb_[slot]; }
    const IndexBuffer &indexBuffer() const { return ib_; }
    uint32_t shader(uint32_t stage) const { return shader_[stage]; }
    uint32_t inputLayout() const { return inputLayout_; }
    uint32_t topology() const { return topology_; }
private:
    std::array<std::array<ConstantBuffer,16>,3> cb_{};
    std::array<std::array<uint32_t,128>,3> srv_{};
    std::array<std::array<uint32_t,16>,3> sampler_{};
    std::array<std::array<UavBinding,8>,3> uav_{};
    std::array<RenderTarget,8> rt_{};
    DepthTarget ds_{};
    std::array<VertexBuffer,16> vb_{};
    IndexBuffer ib_{};
    std::array<uint32_t,3> shader_{};
    uint32_t inputLayout_=0, topology_=0;
};
