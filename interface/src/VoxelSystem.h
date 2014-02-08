//
//  VoxelSystem.h
//  interface
//
//  Created by Philip on 12/31/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__VoxelSystem__
#define __interface__VoxelSystem__

#include "InterfaceConfig.h"
#include <glm/glm.hpp>

#include <SharedUtil.h>

#include <CoverageMapV2.h>
#include <NodeData.h>
#include <ViewFrustum.h>
#include <VoxelTree.h>
#include <OctreePersistThread.h>

#include "Camera.h"
#include "Util.h"
#include "world.h"
#include "renderer/VoxelShader.h"
#include "PrimitiveRenderer.h"

class ProgramObject;

const int NUM_CHILDREN = 8;


struct VoxelShaderVBOData
{
    float x, y, z; // position
    float s; // size
    unsigned char r,g,b; // color
};


class VoxelSystem : public NodeData, public OctreeElementDeleteHook, public OctreeElementUpdateHook {
    Q_OBJECT

    friend class VoxelHideShowThread;

public:
    VoxelSystem(float treeScale = TREE_SCALE, int maxVoxels = DEFAULT_MAX_VOXELS_PER_SYSTEM);
    ~VoxelSystem();

    void setDataSourceUUID(const QUuid& dataSourceUUID) { _dataSourceUUID = dataSourceUUID; }
    const QUuid&  getDataSourceUUID() const { return _dataSourceUUID; }

    int parseData(const QByteArray& packet);

    virtual void init();
    void simulate(float deltaTime) { }
    void render(bool texture);

    void changeTree(VoxelTree* newTree);
    VoxelTree* getTree() const { return _tree; }
    ViewFrustum* getViewFrustum() const { return _viewFrustum; }
    void setViewFrustum(ViewFrustum* viewFrustum) { _viewFrustum = viewFrustum; }
    unsigned long  getVoxelsUpdated() const { return _voxelsUpdated; }
    unsigned long  getVoxelsRendered() const { return _voxelsInReadArrays; }
    unsigned long  getVoxelsWritten() const { return _voxelsInWriteArrays; }
    unsigned long  getAbandonedVoxels() const { return _freeIndexes.size(); }

    ViewFrustum* getLastCulledViewFrustum() { return &_lastCulledViewFrustum; }

    void writeToSVOFile(const char* filename, VoxelTreeElement* element) const;
    bool readFromSVOFile(const char* filename);
    bool readFromSquareARGB32Pixels(const char* filename);
    bool readFromSchematicFile(const char* filename);

    void setMaxVoxels(int maxVoxels);
    long int getMaxVoxels() const { return _maxVoxels; }
    unsigned long getVoxelMemoryUsageRAM() const { return _memoryUsageRAM; }
    unsigned long getVoxelMemoryUsageVBO() const { return _memoryUsageVBO; }
    bool hasVoxelMemoryUsageGPU() const { return _hasMemoryUsageGPU; }
    unsigned long getVoxelMemoryUsageGPU();

    void killLocalVoxels();
    void redrawInViewVoxels();

    virtual void removeOutOfView();
    virtual void hideOutOfView(bool forceFullFrustum = false);
    bool hasViewChanged();
    bool isViewChanging();

    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                             VoxelDetail& detail, float& distance, BoxFace& face);

    bool findSpherePenetration(const glm::vec3& center, float radius, glm::vec3& penetration);
    bool findCapsulePenetration(const glm::vec3& start, const glm::vec3& end, float radius, glm::vec3& penetration);

    void deleteVoxelAt(float x, float y, float z, float s);
    VoxelTreeElement* getVoxelAt(float x, float y, float z, float s) const;
    void createVoxel(float x, float y, float z, float s,
                     unsigned char red, unsigned char green, unsigned char blue, bool destructive = false);
    void createLine(glm::vec3 point1, glm::vec3 point2, float unitSize, rgbColor color, bool destructive = false);

    void copySubTreeIntoNewTree(VoxelTreeElement* startNode, VoxelSystem* destinationTree, bool rebaseToRoot);
    void copySubTreeIntoNewTree(VoxelTreeElement* startNode, VoxelTree* destinationTree, bool rebaseToRoot);
    void copyFromTreeIntoSubTree(VoxelTree* sourceTree, VoxelTreeElement* destinationNode);

    void recurseTreeWithOperation(RecurseOctreeOperation operation, void* extraData=NULL);

    CoverageMapV2 myCoverageMapV2;
    CoverageMap   myCoverageMap;

    virtual void elementDeleted(OctreeElement* element);
    virtual void elementUpdated(OctreeElement* element);

    bool treeIsBusy() const { return _treeIsBusy; }

    VoxelTreeElement* getVoxelEnclosing(const glm::vec3& point);

signals:
    void importSize(float x, float y, float z);
    void importProgress(int progress);

public slots:
    void nodeAdded(SharedNodePointer node);
    void nodeKilled(SharedNodePointer node);
                        
    void collectStatsForTreesAndVBOs();

    // Methods that recurse tree
    void showAllLocalVoxels();
    void randomizeVoxelColors();
    void falseColorizeRandom();
    void trueColorize();
    void falseColorizeInView();
    void falseColorizeDistanceFromView();
    void falseColorizeRandomEveryOther();
    void falseColorizeOccluded();
    void falseColorizeOccludedV2();
    void falseColorizeBySource();
    void forceRedrawEntireTree();
    void clearAllNodesBufferIndex();
    void cullSharedFaces();
    void showCulledSharedFaces();

    void cancelImport();

    void setDisableFastVoxelPipeline(bool disableFastVoxelPipeline);
    void setUseVoxelShader(bool useVoxelShader);
    void setVoxelsAsPoints(bool voxelsAsPoints);

    void localVoxelCacheLoaded();
    void beginLoadingLocalVoxelCache();

protected:
    float _treeScale;
    unsigned long _maxVoxels;
    VoxelTree* _tree;

    void setupNewVoxelsForDrawing();
    static const bool DONT_BAIL_EARLY; // by default we will bail early, if you want to force not bailing, then use this
    void setupNewVoxelsForDrawingSingleNode(bool allowBailEarly = true);
    void checkForCulling();

    glm::vec3 computeVoxelVertex(const glm::vec3& startVertex, float voxelScale, int index) const;


    virtual void updateArraysDetails(glBufferIndex nodeIndex, const glm::vec3& startVertex,
                                    float voxelScale, const nodeColor& color);
    virtual void copyWrittenDataSegmentToReadArrays(glBufferIndex segmentStart, glBufferIndex segmentEnd);
    virtual void updateVBOSegment(glBufferIndex segmentStart, glBufferIndex segmentEnd);
    virtual void applyScaleAndBindProgram(bool texture);
    virtual void removeScaleAndReleaseProgram(bool texture);

private:
    // disallow copying of VoxelSystem objects
    VoxelSystem(const VoxelSystem&);
    VoxelSystem& operator= (const VoxelSystem&);

    bool _initialized;
    int  _callsToTreesToArrays;
    OctreeElementBag _removedVoxels;

    // Operation functions for tree recursion methods
    static int _nodeCount;
    static bool randomColorOperation(OctreeElement* element, void* extraData);
    static bool falseColorizeRandomOperation(OctreeElement* element, void* extraData);
    static bool trueColorizeOperation(OctreeElement* element, void* extraData);
    static bool falseColorizeInViewOperation(OctreeElement* element, void* extraData);
    static bool falseColorizeDistanceFromViewOperation(OctreeElement* element, void* extraData);
    static bool getDistanceFromViewRangeOperation(OctreeElement* element, void* extraData);
    static bool removeOutOfViewOperation(OctreeElement* element, void* extraData);
    static bool falseColorizeRandomEveryOtherOperation(OctreeElement* element, void* extraData);
    static bool collectStatsForTreesAndVBOsOperation(OctreeElement* element, void* extraData);
    static bool falseColorizeOccludedOperation(OctreeElement* element, void* extraData);
    static bool falseColorizeSubTreeOperation(OctreeElement* element, void* extraData);
    static bool falseColorizeOccludedV2Operation(OctreeElement* element, void* extraData);
    static bool falseColorizeBySourceOperation(OctreeElement* element, void* extraData);
    static bool killSourceVoxelsOperation(OctreeElement* element, void* extraData);
    static bool forceRedrawEntireTreeOperation(OctreeElement* element, void* extraData);
    static bool clearAllNodesBufferIndexOperation(OctreeElement* element, void* extraData);
    static bool inspectForExteriorOcclusionsOperation(OctreeElement* element, void* extraData);
    static bool inspectForInteriorOcclusionsOperation(OctreeElement* element, void* extraData);
    static bool clearOcclusionsOperation(OctreeElement* element, void* extraData);
	static bool hideOutOfViewOperation(OctreeElement* element, void* extraData);
    static bool hideAllSubTreeOperation(OctreeElement* element, void* extraData);
    static bool showAllSubTreeOperation(OctreeElement* element, void* extraData);
    static bool showAllLocalVoxelsOperation(OctreeElement* element, void* extraData);
    static bool getVoxelEnclosingOperation(OctreeElement* element, void* extraData);

    int updateNodeInArrays(VoxelTreeElement* node, bool reuseIndex, bool forceDraw);
    int forceRemoveNodeFromArrays(VoxelTreeElement* node);

    void copyWrittenDataToReadArraysFullVBOs();
    void copyWrittenDataToReadArraysPartialVBOs();

    void updateVBOs();

    unsigned long getFreeMemoryGPU();

    // these are kinda hacks, used by getDistanceFromViewRangeOperation() probably shouldn't be here
    static float _maxDistance;
    static float _minDistance;

    GLfloat* _readVerticesArray;
    GLubyte* _readColorsArray;
    GLfloat* _writeVerticesArray;
    GLubyte* _writeColorsArray;
    bool* _writeVoxelDirtyArray;
    bool* _readVoxelDirtyArray;
    unsigned long _voxelsUpdated;
    unsigned long _voxelsInReadArrays;
    unsigned long _voxelsInWriteArrays;
    unsigned long _abandonedVBOSlots;

    bool _writeRenderFullVBO;
    bool _readRenderFullVBO;

    int _setupNewVoxelsForDrawingLastElapsed;
    quint64 _setupNewVoxelsForDrawingLastFinished;
    quint64 _lastViewCulling;
    quint64 _lastViewIsChanging;
    quint64 _lastAudit;
    int _lastViewCullingElapsed;
    bool _hasRecentlyChanged;

    void initVoxelMemory();
    void cleanupVoxelMemory();

    bool _useVoxelShader;
    bool _voxelsAsPoints;
    bool _voxelShaderModeWhenVoxelsAsPointsEnabled;

    GLuint _vboVoxelsID; /// when using voxel shader, we'll use this VBO
    GLuint _vboVoxelsIndicesID;  /// when using voxel shader, we'll use this VBO for our indexes
    VoxelShaderVBOData* _writeVoxelShaderData;
    VoxelShaderVBOData* _readVoxelShaderData;

    GLuint _vboVerticesID;
    GLuint _vboColorsID;

    GLuint _vboIndicesTop;
    GLuint _vboIndicesBottom;
    GLuint _vboIndicesLeft;
    GLuint _vboIndicesRight;
    GLuint _vboIndicesFront;
    GLuint _vboIndicesBack;

    QMutex _bufferWriteLock;
    QMutex _treeLock;

    ViewFrustum _lastKnownViewFrustum;
    ViewFrustum _lastStableViewFrustum;
    ViewFrustum* _viewFrustum;

    ViewFrustum _lastCulledViewFrustum; // used for hide/show visible passes
    bool _culledOnce;

    void setupFaceIndices(GLuint& faceVBOID, GLubyte faceIdentityIndices[]);

    int newTreeToArrays(VoxelTreeElement* currentNode);
    void cleanupRemovedVoxels();

    void copyWrittenDataToReadArrays(bool fullVBOs);

    void updateFullVBOs(); // all voxels in the VBO
    void updatePartialVBOs(); // multiple segments, only dirty voxels

    bool _voxelsDirty;

    static ProgramObject _perlinModulateProgram;
    static ProgramObject _shadowMapProgram;

    int _hookID;
    std::vector<glBufferIndex> _freeIndexes;
    QMutex _freeIndexLock;

    void freeBufferIndex(glBufferIndex index);
    void clearFreeBufferIndexes();
    glBufferIndex getNextBufferIndex();

    bool _falseColorizeBySource;
    QUuid _dataSourceUUID;

    int _voxelServerCount;
    unsigned long _memoryUsageRAM;
    unsigned long _memoryUsageVBO;
    unsigned long _initialMemoryUsageGPU;
    bool _hasMemoryUsageGPU;

    bool _inSetupNewVoxelsForDrawing;
    bool _useFastVoxelPipeline;

    bool _inhideOutOfView;
    bool _treeIsBusy; // is the tree mutex locked? if so, it's busy, and if you can avoid it, don't access the tree

    void lockTree();
    void unlockTree();

    bool _showCulledSharedFaces;                ///< Flag visibility of culled faces
	bool _usePrimitiveRenderer;					///< Flag primitive renderer for use
	PrimitiveRenderer* _renderer;				///< Voxel renderer

	static unsigned char _sOctantIndexToBitMask[8];				///< Map octant index to partition mask
	static unsigned char _sOctantIndexToSharedBitMask[8][8];		///< Map octant indices to shared partition mask

};

#endif
