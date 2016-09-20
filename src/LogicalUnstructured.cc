/*
 * LogicalElement.cc
 *
 *  Created on: Sep 8, 2016
 *      Author: jgraham
 */


#include "LogicalUnstructured.hh"

#include "Vec2.hh"


LogicalUnstructured::LogicalUnstructured(Context ctx, HighLevelRuntime *runtime,
        IndexSpace i_space) :
    LogicalUnstructured(ctx, runtime)
{
    ispace = i_space;
    ispaceID = new IndexSpaceID;
    *ispaceID = ispace.get_id();
}


LogicalUnstructured::LogicalUnstructured(Context ctx, HighLevelRuntime *runtime,
        PhysicalRegion pregion) :
    destroy_ispace(false),
    ispace(pregion.get_logical_region().get_index_space()),
    destroy_fspace(false),
    fspace(pregion.get_logical_region().get_field_space()),
    destroy_lregion(false),
    lregion(pregion.get_logical_region()),
    lpartID(NULL),
    pregion(pregion),
    ctx(ctx),
    runtime(runtime)
{
    ispaceID = new IndexSpaceID;
    *ispaceID = pregion.get_logical_region().get_index_space().get_id();
    fspaceID = new FieldSpaceID;
    *fspaceID = pregion.get_logical_region().get_field_space().get_id();
    lregionID = new RegionTreeID;
    *lregionID = pregion.get_logical_region().get_tree_id();
}


LogicalUnstructured::LogicalUnstructured(Context ctx, HighLevelRuntime *runtime) :
    destroy_ispace(false),
    ispaceID(NULL),
    destroy_fspace(true),
    destroy_lregion(false),
    lregionID(NULL),
    lpartID(NULL),
    ctx(ctx),
    runtime(runtime)
{
    fspace = runtime->create_field_space(ctx);
    runtime->attach_name(fspace, "LogicalElement::fSpace");
    fspaceID = new FieldSpaceID;
    *fspaceID = fspace.get_id();
}


LogicalUnstructured::~LogicalUnstructured() {
/*    if (lregionID != NULL) {
        runtime->destroy_logical_region(ctx, lregion);
        delete lregionID;
    }
    if (lpartID != NULL)
        delete lpartID;
    runtime->destroy_field_space(ctx, fspace);
    delete fspaceID;
    if (ispaceID != NULL) {
        runtime->destroy_index_space(ctx, ispace);
        delete ispaceID;
    }
    if (pregion.is_mapped())
        runtime->unmap_region(ctx, pregion);

*/
/*    if (destroy_lregion)
        runtime->destroy_logical_region(ctx, lregion);
    if (destroy_fspace)
        runtime->destroy_field_space(ctx, fspace);
    if (destroy_ispace)
        runtime->destroy_index_space(ctx, ispace);

    if (lregionID != NULL)
        delete lregionID;
    if (lpartID != NULL)
        delete lpartID;
    delete fspaceID;
    if (ispaceID != NULL)
        delete ispaceID;*/
}


void LogicalUnstructured::partition(Coloring map, bool disjoint)
{
    assert( (ispaceID != NULL) && (lpartID == NULL) );
    IndexPartition part = runtime->create_index_partition(ctx, ispace, map, disjoint);
    runtime->attach_name(part, "LogicalElement::part");
    lpart = runtime->get_logical_partition(ctx, lregion, part);
    lpartID = new RegionTreeID;
    *lpartID = lpart.get_tree_id();
}


void LogicalUnstructured::allocate(int nElements)
{
    assert( (nElements > 0) && (fIDs.size() > 0) && (!pregion.is_mapped())
            && (ispaceID == NULL) && (lregionID == NULL) );

    ispace = runtime->create_index_space(ctx, nElements);
    destroy_ispace = true;
    char buf[43];
    sprintf(buf, "LogicalElement::iSpace %d", nElements);
    runtime->attach_name(ispace, buf);
    IndexAllocator allocator = runtime->create_index_allocator(ctx, ispace);
    ptr_t begin = allocator.alloc(nElements);
    assert(!begin.is_null());
    ispaceID = new IndexSpaceID;
    *ispaceID = ispace.get_id();
    allocate();
}


void LogicalUnstructured::allocate()
{
    assert( (fIDs.size() > 0) && (!pregion.is_mapped())
            && (ispaceID != NULL) && (lregionID == NULL) );
    lregion = runtime->create_logical_region(ctx, ispace, fspace);
    destroy_lregion = true;
    runtime->attach_name(lregion, "LogicalElement::lRegion");
    lregionID = new RegionTreeID;
    *lregionID = lregion.get_tree_id();
}


void LogicalUnstructured::addField(FieldID FID)
{
    assert(lregionID == NULL);
    fIDs.push_back(FID);
}


template <>
void LogicalUnstructured::addField<double2>(FieldID FID)
{
    addField(FID);
    FieldAllocator allocator = runtime->create_field_allocator(ctx, fspace);
    allocator.allocate_field(sizeof(double2), FID);
}


template <>
void LogicalUnstructured::addField<double>(FieldID FID)
{
    addField(FID);
    FieldAllocator allocator = runtime->create_field_allocator(ctx, fspace);
    allocator.allocate_field(sizeof(double), FID);
}


template <>
void LogicalUnstructured::addField<int>(FieldID FID)
{
    addField(FID);
    FieldAllocator allocator = runtime->create_field_allocator(ctx, fspace);
    allocator.allocate_field(sizeof(int), FID);
}

PhysicalRegion LogicalUnstructured::getPRegion()
{
    assert(lregionID != NULL);
    if (!pregion.is_mapped() ) {
        RegionRequirement req(lregion, WRITE_DISCARD, EXCLUSIVE, lregion);
        for (int i=0; i<fIDs.size(); i++)
            req.add_field(fIDs[i]);
        InlineLauncher launcher(req);
        pregion = runtime->map_region(ctx, launcher);
    }
    return pregion;
}


template <>
Double2Accessor LogicalUnstructured::getRegionAccessor<double2>(FieldID FID)
{
    getPRegion();
    return  pregion.get_field_accessor(FID).typeify<double2>();
}


template <>
DoubleAccessor LogicalUnstructured::getRegionAccessor<double>(FieldID FID)
{
    getPRegion();
    return  pregion.get_field_accessor(FID).typeify<double>();
}


template <>
IntAccessor LogicalUnstructured::getRegionAccessor<int>(FieldID FID)
{
    getPRegion();
    return  pregion.get_field_accessor(FID).typeify<int>();
}