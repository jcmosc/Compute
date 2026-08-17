#pragma once

#include <AttributeGraph/AGBase.h>

#if TARGET_OS_MAC
#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFDictionary.h>
#else
#include <SwiftCorelibsCoreFoundation/CFArray.h>
#include <SwiftCorelibsCoreFoundation/CFData.h>
#include <SwiftCorelibsCoreFoundation/CFDictionary.h>
#endif

#include <AttributeGraph/AGAttribute.h>
#include <AttributeGraph/AGAttributeInfo.h>
#include <AttributeGraph/AGAttributeType.h>
#include <AttributeGraph/AGCachedValueOptions.h>
#include <AttributeGraph/AGChangedValue.h>
#include <AttributeGraph/AGComparison.h>
#include <AttributeGraph/AGGraphCounterQueryType.h>
#include <AttributeGraph/AGInputOptions.h>
#include <AttributeGraph/AGSearchOptions.h>
#include <AttributeGraph/AGType.h>
#include <AttributeGraph/AGValue.h>
#include <AttributeGraph/AGWeakAttribute.h>

AG_ASSUME_NONNULL_BEGIN
AG_IMPLICIT_BRIDGING_ENABLED

AG_EXTERN_C_BEGIN

// MARK: CFType

typedef struct AG_BRIDGED_TYPE(id) AGGraphStorage *AGGraphRef AG_SWIFT_NAME(Graph);
typedef void *AGUnownedGraphContextRef AG_SWIFT_STRUCT AG_SWIFT_NAME(UnownedGraphContext);

AG_EXPORT
AG_REFINED_FOR_SWIFT
CFTypeID AGGraphGetTypeID(void) AG_SWIFT_NAME(getter:Graph.typeID());

// MARK: Graph Context

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGGraphRef AGGraphCreate(void) AG_SWIFT_NAME(Graph.init());

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGGraphRef AGGraphCreateShared(AGGraphRef _Nullable graph) AG_SWIFT_NAME(Graph.init(shared:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGUnownedGraphContextRef AGGraphGetGraphContext(AGGraphRef graph) AG_SWIFT_NAME(getter:Graph.graphContext(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGGraphRef AGGraphContextGetGraph(void *context);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphInvalidate(AGGraphRef graph) AG_SWIFT_NAME(Graph.invalidate(self:));

// MARK: User context

AG_EXPORT
AG_REFINED_FOR_SWIFT
const void *_Nullable AGGraphGetContext(AGGraphRef graph) AG_SWIFT_NAME(getter:Graph.context(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetContext(AGGraphRef graph, const void *_Nullable context) AG_SWIFT_NAME(setter:Graph.context(self:_:));

// MARK: Counter

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint64_t AGGraphGetCounter(AGGraphRef graph, AGGraphCounterQueryType query) AG_SWIFT_NAME(Graph.counter(self:for:));

// MARK: Main handler

typedef void (*AGGraphMainThreadHandler)(void (*trampoline_thunk)(const void *trampoline AG_SWIFT_CONTEXT)
                                              AG_SWIFT_CC(swift),
                                          const void *trampoline, const void *_Nullable context AG_SWIFT_CONTEXT)
    AG_SWIFT_CC(swift);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphWithMainThreadHandler(AGGraphRef graph,
                                   void (*body)(const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                                   const void *body_context, AGGraphMainThreadHandler main_thread_handler,
                                   const void *main_thread_handler_context);

// MARK: Subgraphs

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphBeginDeferringSubgraphInvalidation(AGGraphRef graph)
    AG_SWIFT_NAME(Graph.beginDeferringSubgraphInvalidation(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphEndDeferringSubgraphInvalidation(AGGraphRef graph, bool was_deferring)
    AG_SWIFT_NAME(Graph.endDeferringSubgraphInvalidation(self:wasDeferring:));

// MARK: Attribute types

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttributeTypeIndex AGGraphInternAttributeType(
    AGUnownedGraphContextRef graph, AGTypeID type,
    const AGAttributeType *_Nonnull (*_Nonnull make_attribute_type)(const void *_Nullable context AG_SWIFT_CONTEXT)
        AG_SWIFT_CC(swift),
    const void *_Nullable make_attribute_type_context);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphVerifyType(AGAttribute attribute, AGTypeID type) AG_SWIFT_NAME(AnyAttribute.verifyType(self:type:));

// MARK: Attributes

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateAttribute(AGAttributeTypeIndex type_index, const void *body, const void *_Nullable value)
    AG_SWIFT_NAME(AnyAttribute.init(typeIndex:body:value:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGGraphRef AGGraphGetAttributeGraph(AGAttribute attribute) AG_SWIFT_NAME(getter:AnyAttribute.graph(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttributeInfo AGGraphGetAttributeInfo(AGAttribute attribute) AG_SWIFT_NAME(getter:AnyAttribute.info(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttributeFlags AGGraphGetFlags(AGAttribute attribute) AG_SWIFT_NAME(getter:AnyAttribute.flags(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetFlags(AGAttribute attribute, AGAttributeFlags flags)
    AG_SWIFT_NAME(setter:AnyAttribute.flags(self:_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint32_t AGGraphAddInput(AGAttribute attribute, AGAttribute input, AGInputOptions options)
    AG_SWIFT_NAME(AnyAttribute.addInput(self:_:options:));

// MARK: Offset attributes

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateOffsetAttribute(AGAttribute attribute, uint32_t offset);

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateOffsetAttribute2(AGAttribute attribute, uint32_t offset, size_t size);

// MARK: Indirect attributes

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateIndirectAttribute(AGAttribute attribute) AG_SWIFT_NAME(AnyAttribute.createIndirect(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateIndirectAttribute2(AGAttribute attribute, size_t size);

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGGraphGetIndirectAttribute(AGAttribute attribute) AG_SWIFT_NAME(getter:AnyAttribute.source(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetIndirectAttribute(AGAttribute attribute, AGAttribute source)
    AG_SWIFT_NAME(setter:AnyAttribute.source(self:_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphResetIndirectAttribute(AGAttribute attribute, bool non_nil);

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGGraphGetIndirectDependency(AGAttribute attribute);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetIndirectDependency(AGAttribute attribute, AGAttribute dependency);

// MARK: Search

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphSearch(AGAttribute attribute, AGSearchOptions options,
                    bool (*predicate)(AGAttribute attribute, const void *context AG_SWIFT_CONTEXT)
                        AG_SWIFT_CC(swift),
                    const void *predicate_context);

// MARK: Body

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphMutateAttribute(AGAttribute attribute, AGTypeID type, bool invalidating,
                             void (*modify)(void *body, const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                             const void *modify_context);

// MARK: Value

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGChangedValue AGGraphGetValue(AGAttribute attribute, AGValueOptions options, AGTypeID type);

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGWeakChangedValue AGGraphGetWeakValue(AGWeakAttribute attribute, AGValueOptions options, AGTypeID type);

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGChangedValue AGGraphGetInputValue(AGAttribute attribute, AGAttribute input, AGValueOptions options,
                                      AGTypeID type);

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphSetValue(AGAttribute attribute, const void *value, AGTypeID type);

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphHasValue(AGAttribute attribute) AG_SWIFT_NAME(getter:AnyAttribute.hasValue(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGValueState AGGraphGetValueState(AGAttribute attribute) AG_SWIFT_NAME(getter:AnyAttribute.valueState(self:));

typedef AG_OPTIONS(uint32_t, AGGraphUpdateOptions){
    AGGraphUpdateOptionsNone = 0,
    AGGraphUpdateOptionsInTransaction = 1 << 0,
    AGGraphUpdateOptionsAbortIfCancelled = 1 << 1,
    AGGraphUpdateOptionsCancelIfPassedDeadline = 1 << 2,
    AGGraphUpdateOptionsInitializeCleared = 1 << 3,
    AGGraphUpdateOptionsEndDeferringSubgraphInvalidationOnExit = 1 << 4,
};

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphUpdateValue(AGAttribute attribute, AGGraphUpdateOptions options)
    AG_SWIFT_NAME(AnyAttribute.updateValue(self:options:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint32_t AGGraphPrefetchValue(AGAttribute attribute) AG_SWIFT_NAME(AnyAttribute.prefetchValue(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphInvalidateValue(AGAttribute attribute) AG_SWIFT_NAME(AnyAttribute.invalidateValue(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphInvalidateAllValues(AGGraphRef graph) AG_SWIFT_NAME(Graph.invalidateAllValues(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetInvalidationCallback(AGGraphRef graph,
                                     void (*callback)(AGAttribute, const void *context AG_SWIFT_CONTEXT)
                                         AG_SWIFT_CC(swift),
                                     const void *callback_context);

// MARK: Cached value

CF_EXPORT
CF_REFINED_FOR_SWIFT
void *AGGraphReadCachedAttribute(size_t hash, AGTypeID type, const void *body, AGTypeID value_type,
                                  AGCachedValueOptions options, AGAttribute owner, bool *_Nullable changed_out,
                                  uint32_t (*closure)(AGUnownedGraphContextRef graph_context,
                                                      const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                                  const void *closure_context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void *_Nullable AGGraphReadCachedAttributeIfExists(size_t hash, AGTypeID type, const void *body, AGTypeID value_type,
                                                    AGCachedValueOptions options, AGAttribute owner,
                                                    bool *_Nullable changed_out);

// MARK: Update

typedef AG_ENUM(uint32_t, AGGraphUpdateStatus){
    AGGraphUpdateStatusNoChange = 0,
    AGGraphUpdateStatusChanged = 1,
    AGGraphUpdateStatusAborted = 2,
    AGGraphUpdateStatusNeedsCallMainHandler = 3,
};

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetUpdate(const void *update) AG_SWIFT_NAME(Graph.setUpdate(_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
const void *AGGraphClearUpdate(void) AG_SWIFT_NAME(Graph.clearUpdate());

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphCancelUpdate(void) AG_SWIFT_NAME(Graph.cancelUpdate());

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphCancelUpdateIfNeeded(void) AG_SWIFT_NAME(Graph.cancelUpdateIfNeeded());

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphUpdateWasCancelled(void) AG_SWIFT_NAME(getter:Graph.updateWasCancelled());

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint64_t AGGraphGetDeadline(AGGraphRef graph) AG_SWIFT_NAME(getter:Graph.deadline(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetDeadline(AGGraphRef graph, uint64_t deadline) AG_SWIFT_NAME(setter:Graph.deadline(self:_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphHasDeadlinePassed(void) AG_SWIFT_NAME(getter:Graph.hasDeadlinePassed());

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetNeedsUpdate(AGGraphRef graph) AG_SWIFT_NAME(Graph.setNeedsUpdate(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphWithUpdate(AGAttribute attribute, void (*body)(const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                        const void *body_context);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphWithoutUpdate(void (*body)(const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                           const void *body_context);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetUpdateCallback(AGGraphRef graph,
                               void (*callback)(const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                               const void *callback_context);

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGGraphGetCurrentAttribute(void);

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphCurrentAttributeWasModified(void) AG_SWIFT_NAME(getter:AnyAttribute.currentWasModified());

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphAnyInputsChanged(const AGAttribute *AG_COUNTED_BY(count) exclude_attributes, size_t count);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void *_Nullable AGGraphGetOutputValue(AGTypeID type);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetOutputValue(const void *value, AGTypeID type);

// MARK: Description

#if TARGET_OS_MAC
AG_EXPORT
AG_REFINED_FOR_SWIFT
CFTypeRef _Nullable AGGraphDescription(AGGraphRef _Nullable graph, CFDictionaryRef options)
    AG_SWIFT_NAME(Graph.description(_:options:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphArchiveJSON(const char *_Nullable filename) AG_SWIFT_NAME(Graph.archiveJSON(name:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphArchiveJSON2(const char *filename, bool exclude_values)
    AG_SWIFT_NAME(Graph.archiveJSON(name:excludeValues:));
#endif

AG_EXTERN_C_END

AG_IMPLICIT_BRIDGING_DISABLED
AG_ASSUME_NONNULL_END
