//
//  GL45BackendTexture.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 1/19/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL45Backend.h"

#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <unordered_map>
#include <glm/gtx/component_wise.hpp>

#include <QtCore/QThread>

#include "../gl/GLTexelFormat.h"

using namespace gpu;
using namespace gpu::gl;
using namespace gpu::gl45;

#ifdef THREADED_TEXTURE_TRANSFER
#define SPARSE_TEXTURES 1
#else
#define SPARSE_TEXTURES 0
#endif

// Allocate 1 MB of buffer space for paged transfers
#define DEFAULT_PAGE_BUFFER_SIZE (1024*1024)

using GL45Texture = GL45Backend::GL45Texture;

static std::map<uint16_t, std::unordered_set<GL45Texture*>> texturesByMipCounts;

GLTexture* GL45Backend::syncGPUObject(const TexturePointer& texture, bool transfer) {
    return GL45Texture::sync<GL45Texture>(*this, texture, transfer);
}

using SparseInfo = GL45Backend::GL45Texture::SparseInfo;

SparseInfo::SparseInfo(GL45Texture& texture)
    : _texture(texture) {
}

void SparseInfo::update() {
    glGetTextureParameterIuiv(_texture._id, GL_NUM_SPARSE_LEVELS_ARB, &_maxSparseLevel);
    GLenum internalFormat = gl::GLTexelFormat::evalGLTexelFormat(_texture._gpuObject.getTexelFormat(), _texture._gpuObject.getTexelFormat()).internalFormat;
    ivec3 pageSize;
    glGetInternalformativ(_texture._target, internalFormat, GL_VIRTUAL_PAGE_SIZE_X_ARB, 1, &pageSize.x);
    glGetInternalformativ(_texture._target, internalFormat, GL_VIRTUAL_PAGE_SIZE_Y_ARB, 1, &pageSize.y);
    glGetInternalformativ(_texture._target, internalFormat, GL_VIRTUAL_PAGE_SIZE_Z_ARB, 1, &pageSize.z);
    _pageDimensions = uvec3(pageSize);
    _pageBytes = _texture._gpuObject.getTexelFormat().getSize();
    _pageBytes *= _pageDimensions.x * _pageDimensions.y * _pageDimensions.z;

    for (uint16_t mipLevel = 0; mipLevel <= _maxSparseLevel; ++mipLevel) {
        auto mipDimensions = _texture._gpuObject.evalMipDimensions(mipLevel);
        auto mipPageCount = getPageCount(mipDimensions);
        _maxPages += mipPageCount;
    }
    if (_texture._target == GL_TEXTURE_CUBE_MAP) {
        _maxPages *= GLTexture::CUBE_NUM_FACES;
    }
}

uvec3 SparseInfo::getPageCounts(const uvec3& dimensions) const {
    auto result = (dimensions / _pageDimensions) +
        glm::clamp(dimensions % _pageDimensions, glm::uvec3(0), glm::uvec3(1));
    return result;


}

uint32_t SparseInfo::getPageCount(const uvec3& dimensions) const {
    auto pageCounts = getPageCounts(dimensions);
    return pageCounts.x * pageCounts.y * pageCounts.z;
}

using TransferState = GL45Backend::GL45Texture::TransferState;

TransferState::TransferState(GL45Texture& texture) : _texture(texture) {
}

void TransferState::updateMip() {
    _mipDimensions = _texture._gpuObject.evalMipDimensions(_mipLevel);
    _mipOffset = uvec3();
    if (!_texture._gpuObject.isStoredMipFaceAvailable(_mipLevel, _face)) {
        _srcPointer = nullptr;
        return;
    }

    auto mip = _texture._gpuObject.accessStoredMipFace(_mipLevel, _face);
    _texelFormat = gl::GLTexelFormat::evalGLTexelFormat(_texture._gpuObject.getTexelFormat(), mip->getFormat());
    _srcPointer = mip->readData();
    _bytesPerLine = (uint32_t)mip->getSize() / _mipDimensions.y;
    _bytesPerPixel = _bytesPerLine / _mipDimensions.x;
}

bool TransferState::increment() {
    const SparseInfo& sparse = _texture._sparseInfo;
    if ((_mipOffset.x + sparse._pageDimensions.x) < _mipDimensions.x) {
        _mipOffset.x += sparse._pageDimensions.x;
        return true;
    }

    if ((_mipOffset.y + sparse._pageDimensions.y) < _mipDimensions.y) {
        _mipOffset.x = 0;
        _mipOffset.y += sparse._pageDimensions.y;
        return true;
    }

    if (_mipOffset.z + sparse._pageDimensions.z < _mipDimensions.z) {
        _mipOffset.x = 0;
        _mipOffset.y = 0;
        ++_mipOffset.z;
        return true;
    }

    // Done with this mip?, move on to the next mip 
    if (_mipLevel + 1 < _texture.usedMipLevels()) {
        _mipOffset = uvec3(0);
        ++_mipLevel;
        updateMip();
        return true;
    }

    uint8_t maxFace = (uint8_t)((_texture._target == GL_TEXTURE_CUBE_MAP) ? GLTexture::CUBE_NUM_FACES : 1);
    uint8_t nextFace = _face + 1;
    // Done with this face?  Move on to the next
    if (nextFace < maxFace) {
        ++_face;
        _mipOffset = uvec3(0);
        _mipLevel = 0;
        updateMip();
        return true;
    }

    return false;
}

#define DEFAULT_GL_PIXEL_ALIGNMENT 4
void TransferState::populatePage(std::vector<uint8_t>& buffer) {
    uvec3 pageSize = currentPageSize();
    auto bytesPerPageLine = _bytesPerPixel * pageSize.x;
    if (0 != (bytesPerPageLine % DEFAULT_GL_PIXEL_ALIGNMENT)) {
        bytesPerPageLine += DEFAULT_GL_PIXEL_ALIGNMENT - (bytesPerPageLine % DEFAULT_GL_PIXEL_ALIGNMENT);
        assert(0 == (bytesPerPageLine % DEFAULT_GL_PIXEL_ALIGNMENT));
    }
    auto totalPageSize = bytesPerPageLine * pageSize.y;
    if (totalPageSize > buffer.size()) {
        buffer.resize(totalPageSize);
    }
    uint8_t* dst = &buffer[0];
    for (uint32_t y = 0; y < pageSize.y; ++y) {
        uint32_t srcOffset = (_bytesPerLine * (_mipOffset.y + y)) + (_bytesPerPixel * _mipOffset.x);
        uint32_t dstOffset = bytesPerPageLine * y;
        memcpy(dst + dstOffset, _srcPointer + srcOffset, pageSize.x * _bytesPerPixel);
    }
}

uvec3 TransferState::currentPageSize() const {
    return glm::clamp(_mipDimensions - _mipOffset, uvec3(1), _texture._sparseInfo._pageDimensions);
}

GLuint GL45Texture::allocate(const Texture& texture) {
    GLuint result;
    glCreateTextures(getGLTextureType(texture), 1, &result);
    return result;
}

GLuint GL45Backend::getTextureID(const TexturePointer& texture, bool transfer) {
    return GL45Texture::getId<GL45Texture>(*this, texture, transfer);
}

GL45Texture::GL45Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture, bool transferrable)
    : GLTexture(backend, texture, allocate(texture), transferrable), _sparseInfo(*this), _transferState(*this) {
#if SPARSE_TEXTURES
    _sparse = _transferrable;
#endif
    if (_sparse) {
        glTextureParameteri(_id, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
    }
}

GL45Texture::~GL45Texture() {
    if (_sparse) {
        auto mipLevels = usedMipLevels();
        if (texturesByMipCounts.count(mipLevels)) {
            auto& textures = texturesByMipCounts[mipLevels];
            textures.erase(this);
            if (textures.empty()) {
                texturesByMipCounts.erase(mipLevels);
            }
        }

        auto originalAllocatedPages = _allocatedPages;
        auto maxSparseMip = std::min<uint16_t>(_maxMip, _sparseInfo._maxSparseLevel);
        uint8_t maxFace = (uint8_t)((_target == GL_TEXTURE_CUBE_MAP) ? GLTexture::CUBE_NUM_FACES : 1);
        for (uint16_t mipLevel = _minMip; mipLevel <= maxSparseMip; ++mipLevel) {
            auto mipDimensions = _gpuObject.evalMipDimensions(mipLevel);
            auto deallocatedPages = _sparseInfo.getPageCount(mipDimensions);
            for (uint8_t face = 0; face < maxFace; ++face) {
                glTexturePageCommitmentEXT(_id, mipLevel, 0, 0, face, mipDimensions.x, mipDimensions.y, mipDimensions.z, GL_FALSE);
                assert(deallocatedPages <= _allocatedPages);
                _allocatedPages -= deallocatedPages;
            }
        }
        if (0 != _allocatedPages) {
            auto maxSize = _gpuObject.evalMipDimensions(0);
            qDebug() << "Allocated pages remaining " << _id << " " << _allocatedPages;
            qDebug() << originalAllocatedPages;
        }
    }
}

void GL45Texture::withPreservedTexture(std::function<void()> f) const {
    f();
}

void GL45Texture::generateMips() const {
    glGenerateTextureMipmap(_id);
    (void)CHECK_GL_ERROR();
}

void GL45Texture::allocateStorage() const {
    GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat());
    glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, 0);
    glTextureParameteri(_id, GL_TEXTURE_MAX_LEVEL, _maxMip - _minMip);
    if (_gpuObject.getTexelFormat().isCompressed()) {
        qFatal("Compressed textures not yet supported");
    }
    // Get the dimensions, accounting for the downgrade level
    Vec3u dimensions = _gpuObject.evalMipDimensions(_minMip);
    glTextureStorage2D(_id, usedMipLevels(), texelFormat.internalFormat, dimensions.x, dimensions.y);
    (void)CHECK_GL_ERROR();
}

void GL45Texture::updateSize() const {
    if (_gpuObject.getTexelFormat().isCompressed()) {
        qFatal("Compressed textures not yet supported");
    }

    if (_transferrable) {
        setSize(_allocatedPages * _sparseInfo._pageBytes);
    } else {
        setSize(_virtualSize);
    }
}

void GL45Texture::startTransfer() {
    Parent::startTransfer();
    _sparseInfo.update();
    _transferState.updateMip();
}

bool GL45Texture::continueTransfer() {
    static std::vector<uint8_t> buffer;
    if (buffer.empty()) {
        buffer.resize(DEFAULT_PAGE_BUFFER_SIZE);
    }
    uvec3 pageSize = _transferState.currentPageSize();
    uvec3 offset = _transferState._mipOffset;

    // FIXME we should be using the DSA for all of this
    if (_sparse && _transferState._mipLevel <= _sparseInfo._maxSparseLevel) {
        if (_allocatedPages > _sparseInfo._maxPages) {
            qDebug() << "Exceeded max page allocation!";
        }
        glBindTexture(_target, _id);
        // FIXME we should be using glTexturePageCommitmentEXT, but for some reason it causes out of memory errors.
        // Either I'm not understanding how it should work or there's a driver bug.
        glTexPageCommitmentARB(_target, _transferState._mipLevel,
            offset.x, offset.y, _transferState._face,
            pageSize.x, pageSize.y, pageSize.z,
            GL_TRUE);
        ++_allocatedPages;
    }

    if (_transferState._srcPointer) {
        // Transfer the mip data
        _transferState.populatePage(buffer);
        if (GL_TEXTURE_2D == _target) {
            glTextureSubImage2D(_id, _transferState._mipLevel,
                offset.x, offset.y,
                pageSize.x, pageSize.y,
                _transferState._texelFormat.format, _transferState._texelFormat.type, &buffer[0]);
        } else if (GL_TEXTURE_CUBE_MAP == _target) {
            auto target = CUBE_FACE_LAYOUT[_transferState._face];
            // DSA ARB does not work on AMD, so use EXT
            // glTextureSubImage3D(_id, mipLevel, 0, 0, face, size.x, size.y, 1, texelFormat.format, texelFormat.type, mip->readData());
            glTextureSubImage2DEXT(_id, target, _transferState._mipLevel,
                offset.x, offset.y,
                pageSize.x, pageSize.y,
                _transferState._texelFormat.format, _transferState._texelFormat.type, &buffer[0]);
        }
    }

    serverWait();
    auto currentMip = _transferState._mipLevel;
    auto result = _transferState.increment();
    if (_transferState._mipLevel != currentMip && currentMip <= _sparseInfo._maxSparseLevel) {
        auto mipDimensions = _gpuObject.evalMipDimensions(currentMip);
        auto mipExpectedPages = _sparseInfo.getPageCount(mipDimensions);
        auto newPages = _allocatedPages - _lastMipAllocatedPages;
        if (newPages != mipExpectedPages) {
            qWarning() << "Unexpected page allocation size... " << newPages << " " << mipExpectedPages;
        }
        _lastMipAllocatedPages = _allocatedPages;
    }
    return result;
}

void GL45Texture::finishTransfer() {
    Parent::finishTransfer();
}

void GL45Texture::syncSampler() const {
    const Sampler& sampler = _gpuObject.getSampler();

    const auto& fm = FILTER_MODES[sampler.getFilter()];
    glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, fm.minFilter);
    glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, fm.magFilter);

    if (sampler.doComparison()) {
        glTextureParameteri(_id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        glTextureParameteri(_id, GL_TEXTURE_COMPARE_FUNC, COMPARISON_TO_GL[sampler.getComparisonFunction()]);
    } else {
        glTextureParameteri(_id, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }

    glTextureParameteri(_id, GL_TEXTURE_WRAP_S, WRAP_MODES[sampler.getWrapModeU()]);
    glTextureParameteri(_id, GL_TEXTURE_WRAP_T, WRAP_MODES[sampler.getWrapModeV()]);
    glTextureParameteri(_id, GL_TEXTURE_WRAP_R, WRAP_MODES[sampler.getWrapModeW()]);
    glTextureParameterfv(_id, GL_TEXTURE_BORDER_COLOR, (const float*)&sampler.getBorderColor());
    auto baseMip = std::max<uint16_t>(sampler.getMipOffset(), _minMip);
    glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, baseMip);
    glTextureParameterf(_id, GL_TEXTURE_MIN_LOD, (float)sampler.getMinMip());
    glTextureParameterf(_id, GL_TEXTURE_MAX_LOD, (sampler.getMaxMip() == Sampler::MAX_MIP_LEVEL ? 1000.f : sampler.getMaxMip()));
    glTextureParameterf(_id, GL_TEXTURE_MAX_ANISOTROPY_EXT, sampler.getMaxAnisotropy());
}

void GL45Texture::postTransfer() {
    Parent::postTransfer();
    if (_transferrable) {
        auto mipLevels = usedMipLevels();
        if (mipLevels > 1 && _minMip < _sparseInfo._maxSparseLevel) {
            auto& textureMap = texturesByMipCounts;
            texturesByMipCounts[mipLevels].insert(this);
        }
    }
}

void GL45Texture::stripToMip(uint16_t newMinMip) {
    if (!_sparse) {
        return;
    }

    if (newMinMip < _minMip) {
        qWarning() << "Cannot decrease the min mip";
        return;
    }

    if (newMinMip >= _sparseInfo._maxSparseLevel) {
        qWarning() << "Cannot increase the min mip into the mip tail";
        return;
    }

    auto mipLevels = usedMipLevels();
    assert(0 != texturesByMipCounts.count(mipLevels));
    assert(0 != texturesByMipCounts[mipLevels].count(this));
    texturesByMipCounts[mipLevels].erase(this);
    if (texturesByMipCounts[mipLevels].empty()) {
        texturesByMipCounts.erase(mipLevels);
    }

    // FIXME this shouldn't be necessary should it?
#if 1
    glGenerateTextureMipmap(_id);
#else
    static GLuint framebuffers[2] = { 0, 0 };
    static std::once_flag initFramebuffers;
    std::call_once(initFramebuffers, [&] {
        glCreateFramebuffers(2, framebuffers);
    });
    auto readSize = _gpuObject.evalMipDimensions(_minMip);
    auto drawSize = _gpuObject.evalMipDimensions(newMinMip);
    glNamedFramebufferTexture(framebuffers[0], GL_COLOR_ATTACHMENT0, _id, _minMip);
    glNamedFramebufferTexture(framebuffers[1], GL_COLOR_ATTACHMENT0, _id, newMinMip);
    glBlitNamedFramebuffer(framebuffers[0], framebuffers[1],
        0, 0, readSize.x, readSize.y,
        0, 0, drawSize.x, drawSize.y,
        GL_COLOR_BUFFER_BIT, GL_LINEAR);
#endif

    uint8_t maxFace = (uint8_t)((_target == GL_TEXTURE_CUBE_MAP) ? GLTexture::CUBE_NUM_FACES : 1);
    for (uint16_t mip = _minMip; mip < newMinMip; ++mip) {
        auto mipDimensions = _gpuObject.evalMipDimensions(mip);
        auto deallocatedPages = _sparseInfo.getPageCount(mipDimensions);
        for (uint8_t face = 0; face < maxFace; ++face) {
            glTexturePageCommitmentEXT(_id, mip,
                0, 0, face,
                mipDimensions.x, mipDimensions.y, mipDimensions.z,
                GL_FALSE);
            assert(deallocatedPages < _allocatedPages);
            _allocatedPages -= deallocatedPages;
        }
    }

    _minMip = newMinMip;
    // Re-sync the sampler to force access to the new mip level
    syncSampler();
    size_t oldSize = _size;
    updateSize();
    size_t newSize = _size;
    if (newSize > oldSize) {
        qDebug() << "WTF";
        qDebug() << "\told size " << oldSize;
        qDebug() << "\tnew size " << newSize;
    }

    // Re-insert into the texture-by-mips map if appropriate
    mipLevels = usedMipLevels();
    if (mipLevels > 1 && _minMip < _sparseInfo._maxSparseLevel) {
        auto& textureMap = texturesByMipCounts;
        texturesByMipCounts[mipLevels].insert(this);
    }
}

void GL45Texture::updateMips() {
    if (!_sparse) {
        return;
    }
    bool modified = false;
    auto newMinMip = std::min<uint16_t>(_gpuObject.minMip(), _sparseInfo._maxSparseLevel);
    if (_minMip < newMinMip) {
        stripToMip(newMinMip);
    }
}

void GL45Texture::derez() {
    if (!_sparse) {
        return;
    }
    assert(_minMip < _sparseInfo._maxSparseLevel);
    assert(_minMip < _maxMip);
    assert(_transferrable);
    stripToMip(_minMip + 1);
}

void GL45Backend::derezTextures() const {
    if (GLTexture::getMemoryPressure() < 1.0f) {
        return;
    }
    qDebug() << "Allowed texture memory " << Texture::getAllowedGPUMemoryUsage();
    qDebug() << "Used texture memory " << Context::getTextureGPUMemoryUsage();

    if (texturesByMipCounts.empty()) {
        qDebug() << "No available textures to derez";
        return;
    }

    auto mipLevel = texturesByMipCounts.rbegin()->first;
    if (mipLevel <= 1) {
        qDebug() << "Max mip levels " << mipLevel;
        return;
    }

    auto& textureMap = texturesByMipCounts;
    auto newMipLevel = mipLevel - 1;
    qDebug() << "Derez a texture";
    GL45Texture* targetTexture = nullptr;
    {
        auto& textures = texturesByMipCounts[mipLevel];
        assert(!textures.empty());
        targetTexture = *textures.begin();
    }
    targetTexture->derez();

    qDebug() << "New Used texture memory " << Context::getTextureGPUMemoryUsage();
}
