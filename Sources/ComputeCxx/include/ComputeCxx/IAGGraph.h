#pragma once

#include <ComputeCxx/IAGBase.h>

#if TARGET_OS_MAC
#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFDictionary.h>
#else
#include <SwiftCorelibsCoreFoundation/CFArray.h>
#include <SwiftCorelibsCoreFoundation/CFData.h>
#include <SwiftCorelibsCoreFoundation/CFDictionary.h>
#endif

#include <ComputeCxx/IAGAttribute.h>
#include <ComputeCxx/IAGAttributeInfo.h>
#include <ComputeCxx/IAGAttributeType.h>
#include <ComputeCxx/IAGCachedValueOptions.h>
#include <ComputeCxx/IAGChangedValue.h>
#include <ComputeCxx/IAGComparison.h>
#include <ComputeCxx/IAGGraphCounterQueryType.h>
#include <ComputeCxx/IAGInputOptions.h>
#include <ComputeCxx/IAGSearchOptions.h>
#include <ComputeCxx/IAGType.h>
#include <ComputeCxx/IAGValue.h>
#include <ComputeCxx/IAGWeakAttribute.h>

IAG_ASSUME_NONNULL_BEGIN
IAG_IMPLICIT_BRIDGING_ENABLED

IAG_EXTERN_C_BEGIN

// MARK: CFType

typedef struct IAG_BRIDGED_TYPE(id) IAGGraphStorage *IAGGraphRef IAG_SWIFT_NAME(Graph);
typedef void *IAGUnownedGraphContextRef IAG_SWIFT_STRUCT IAG_SWIFT_NAME(UnownedGraphContext);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
CFTypeID IAGGraphGetTypeID(void) IAG_SWIFT_NAME(getter:IAGGraphRef.typeID());

// MARK: Graph Context

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGGraphRef IAGGraphCreate(void) IAG_SWIFT_NAME(IAGGraphRef.init());

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGGraphRef IAGGraphCreateShared(IAGGraphRef _Nullable graph) IAG_SWIFT_NAME(IAGGraphRef.init(shared:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGUnownedGraphContextRef IAGGraphGetGraphContext(IAGGraphRef graph)
    IAG_SWIFT_NAME(getter:IAGGraphRef.graphContext(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGGraphRef IAGGraphContextGetGraph(void *context);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphInvalidate(IAGGraphRef graph) IAG_SWIFT_NAME(IAGGraphRef.invalidate(self:));

// MARK: User context

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
const void *_Nullable IAGGraphGetContext(IAGGraphRef graph) IAG_SWIFT_NAME(getter:IAGGraphRef.context(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphSetContext(IAGGraphRef graph, const void *_Nullable context)
    IAG_SWIFT_NAME(setter:IAGGraphRef.context(self:_:));

// MARK: Counter

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
uint64_t IAGGraphGetCounter(IAGGraphRef graph, IAGGraphCounterQueryType query)
    IAG_SWIFT_NAME(IAGGraphRef.counter(self:for:));

// MARK: Main handler

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphWithMainThreadHandler(IAGGraphRef graph,
                                  void (*body)(const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                                  const void *body_context,
                                  void (*main_thread_handler)(void (*trampoline_thunk)(const void *),
                                                              const void *trampoline,
                                                              const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                                  const void *main_thread_handler_context);

// MARK: Subgraphs

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGGraphBeginDeferringSubgraphInvalidation(IAGGraphRef graph)
    IAG_SWIFT_NAME(IAGGraphRef.beginDeferringSubgraphInvalidation(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphEndDeferringSubgraphInvalidation(IAGGraphRef graph, bool was_deferring)
    IAG_SWIFT_NAME(IAGGraphRef.endDeferringSubgraphInvalidation(self:wasDeferring:));

// MARK: Attribute types

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
uint32_t IAGGraphInternAttributeType(IAGUnownedGraphContextRef graph, IAGTypeID type,
                                    const IAGAttributeType *_Nonnull (*_Nonnull make_attribute_type)(
                                        const void *_Nullable context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                                    const void *_Nullable make_attribute_type_context);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphVerifyType(IAGAttribute attribute, IAGTypeID type) IAG_SWIFT_NAME(IAGAttribute.verifyType(self:type:));

// MARK: Attributes

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGAttribute IAGGraphCreateAttribute(uint32_t type_id, const void *body, const void *_Nullable value)
    IAG_SWIFT_NAME(IAGAttribute.init(type:body:value:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGGraphRef IAGGraphGetAttributeGraph(IAGAttribute attribute) IAG_SWIFT_NAME(getter:IAGAttribute.graph(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGAttributeInfo IAGGraphGetAttributeInfo(IAGAttribute attribute) IAG_SWIFT_NAME(getter:IAGAttribute.info(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGAttributeFlags IAGGraphGetFlags(IAGAttribute attribute) IAG_SWIFT_NAME(getter:IAGAttribute.flags(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphSetFlags(IAGAttribute attribute, IAGAttributeFlags flags) IAG_SWIFT_NAME(setter:IAGAttribute.flags(self:_:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
uint32_t IAGGraphAddInput(IAGAttribute attribute, IAGAttribute input, IAGInputOptions options)
    IAG_SWIFT_NAME(IAGAttribute.addInput(self:_:options:));

// MARK: Offset attributes

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGAttribute IAGGraphCreateOffsetAttribute(IAGAttribute attribute, uint32_t offset);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGAttribute IAGGraphCreateOffsetAttribute2(IAGAttribute attribute, uint32_t offset, size_t size);

// MARK: Indirect attributes

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGAttribute IAGGraphCreateIndirectAttribute(IAGAttribute attribute) IAG_SWIFT_NAME(IAGAttribute.createIndirect(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGAttribute IAGGraphCreateIndirectAttribute2(IAGAttribute attribute, size_t size);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGAttribute IAGGraphGetIndirectAttribute(IAGAttribute attribute) IAG_SWIFT_NAME(getter:IAGAttribute.source(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphSetIndirectAttribute(IAGAttribute attribute, IAGAttribute source)
    IAG_SWIFT_NAME(setter:IAGAttribute.source(self:_:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphResetIndirectAttribute(IAGAttribute attribute, bool non_nil);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGAttribute IAGGraphGetIndirectDependency(IAGAttribute attribute);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphSetIndirectDependency(IAGAttribute attribute, IAGAttribute dependency);

// MARK: Search

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGGraphSearch(IAGAttribute attribute, IAGSearchOptions options,
                   bool (*predicate)(IAGAttribute attribute, const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                   const void *predicate_context);

// MARK: Body

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphMutateAttribute(IAGAttribute attribute, IAGTypeID type, bool invalidating,
                            void (*modify)(void *body, const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                            const void *modify_context);

// MARK: Value

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGChangedValue IAGGraphGetValue(IAGAttribute attribute, IAGValueOptions options, IAGTypeID type);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGWeakChangedValue IAGGraphGetWeakValue(IAGWeakAttribute attribute, IAGValueOptions options, IAGTypeID type);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGChangedValue IAGGraphGetInputValue(IAGAttribute attribute, IAGAttribute input, IAGValueOptions options, IAGTypeID type);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGGraphSetValue(IAGAttribute attribute, const void *value, IAGTypeID type);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGGraphHasValue(IAGAttribute attribute) IAG_SWIFT_NAME(getter:IAGAttribute.hasValue(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGValueState IAGGraphGetValueState(IAGAttribute attribute) IAG_SWIFT_NAME(getter:IAGAttribute.valueState(self:));

typedef IAG_OPTIONS(uint32_t, IAGGraphUpdateOptions) {
    IAGGraphUpdateOptionsNone = 0,
    IAGGraphUpdateOptionsInTransaction = 1 << 0,
    IAGGraphUpdateOptionsAbortIfCancelled = 1 << 1,
    IAGGraphUpdateOptionsCancelIfPassedDeadline = 1 << 2,
    IAGGraphUpdateOptionsInitializeCleared = 1 << 3,
    IAGGraphUpdateOptionsEndDeferringSubgraphInvalidationOnExit = 1 << 4,
};

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphUpdateValue(IAGAttribute attribute, IAGGraphUpdateOptions options)
    IAG_SWIFT_NAME(IAGAttribute.updateValue(self:options:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
uint32_t IAGGraphPrefetchValue(IAGAttribute attribute) IAG_SWIFT_NAME(IAGAttribute.prefetchValue(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphInvalidateValue(IAGAttribute attribute) IAG_SWIFT_NAME(IAGAttribute.invalidateValue(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphInvalidateAllValues(IAGGraphRef graph) IAG_SWIFT_NAME(IAGGraphRef.invalidateAllValues(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphSetInvalidationCallback(IAGGraphRef graph,
                                    void (*callback)(IAGAttribute, const void *context IAG_SWIFT_CONTEXT)
                                        IAG_SWIFT_CC(swift),
                                    const void *callback_context);

// MARK: Cached value

CF_EXPORT
CF_REFINED_FOR_SWIFT
void *IAGGraphReadCachedAttribute(size_t hash, IAGTypeID type, const void *body, IAGTypeID value_type,
                                 IAGCachedValueOptions options, IAGAttribute owner, bool *_Nullable changed_out,
                                 uint32_t (*closure)(IAGUnownedGraphContextRef graph_context,
                                                     const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                                 const void *closure_context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void *_Nullable IAGGraphReadCachedAttributeIfExists(size_t hash, IAGTypeID type, const void *body, IAGTypeID value_type,
                                                   IAGCachedValueOptions options, IAGAttribute owner,
                                                   bool *_Nullable changed_out);

// MARK: Update

typedef IAG_ENUM(uint32_t, IAGGraphUpdateStatus) {
    IAGGraphUpdateStatusNoChange = 0,
    IAGGraphUpdateStatusChanged = 1,
    IAGGraphUpdateStatusAborted = 2,
    IAGGraphUpdateStatusNeedsCallMainHandler = 3,
};

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphSetUpdate(const void *update) IAG_SWIFT_NAME(IAGGraphRef.setUpdate(_:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
const void *IAGGraphClearUpdate(void) IAG_SWIFT_NAME(IAGGraphRef.clearUpdate());

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphCancelUpdate(void) IAG_SWIFT_NAME(IAGGraphRef.cancelUpdate());

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGGraphCancelUpdateIfNeeded(void) IAG_SWIFT_NAME(IAGGraphRef.cancelUpdateIfNeeded());

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGGraphUpdateWasCancelled(void) IAG_SWIFT_NAME(getter:IAGGraphRef.updateWasCancelled());

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
uint64_t IAGGraphGetDeadline(IAGGraphRef graph) IAG_SWIFT_NAME(getter:IAGGraphRef.deadline(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphSetDeadline(IAGGraphRef graph, uint64_t deadline) IAG_SWIFT_NAME(setter:IAGGraphRef.deadline(self:_:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGGraphHasDeadlinePassed(void) IAG_SWIFT_NAME(getter:IAGGraphRef.hasDeadlinePassed());

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphSetNeedsUpdate(IAGGraphRef graph) IAG_SWIFT_NAME(IAGGraphRef.setNeedsUpdate(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphWithUpdate(IAGAttribute attribute, void (*body)(const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                       const void *body_context);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphWithoutUpdate(void (*body)(const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                          const void *body_context);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphSetUpdateCallback(IAGGraphRef graph,
                              void (*callback)(const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                              const void *callback_context);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGAttribute IAGGraphGetCurrentAttribute(void);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGGraphCurrentAttributeWasModified(void) IAG_SWIFT_NAME(getter:IAGAttribute.currentWasModified());

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGGraphAnyInputsChanged(const IAGAttribute *IAG_COUNTED_BY(count) exclude_attributes, size_t count);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void *_Nullable IAGGraphGetOutputValue(IAGTypeID type);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphSetOutputValue(const void *value, IAGTypeID type);

// MARK: Description

#if TARGET_OS_MAC
IAG_EXPORT
IAG_REFINED_FOR_SWIFT
CFTypeRef _Nullable IAGGraphDescription(IAGGraphRef _Nullable graph, CFDictionaryRef options)
    IAG_SWIFT_NAME(IAGGraphRef.description(_:options:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphArchiveJSON(const char *_Nullable filename) IAG_SWIFT_NAME(IAGGraphRef.archiveJSON(name:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphArchiveJSON2(const char *filename, bool exclude_values)
    IAG_SWIFT_NAME(IAGGraphRef.archiveJSON(name:excludeValues:));
#endif

IAG_EXTERN_C_END

IAG_IMPLICIT_BRIDGING_DISABLED
IAG_ASSUME_NONNULL_END
