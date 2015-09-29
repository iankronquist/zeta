#include "vm.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

//============================================================================
// VM core
//============================================================================

/// Global VM instance
vm_t vm;

/// Boolean constant values
const value_t VAL_FALSE = { 0, TAG_BOOL };
const value_t VAL_TRUE = { 1, TAG_BOOL };

/// Shape of array objects
shapeidx_t SHAPE_ARRAY;

/// Shape of string objects
shapeidx_t SHAPE_STRING;

value_t value_from_heapptr(heapptr_t v, tag_t tag)
{
    value_t val;
    val.word.heapptr = v;
    val.tag = tag;
    return val; 
}

value_t value_from_int64(int64_t v)
{
    value_t val;
    val.word.int64 = v;
    val.tag = TAG_INT64;
    return val;
}

bool value_equals(value_t this, value_t that)
{
    if (this.tag != that.tag)
        return false;

    if (this.word.int64 != that.word.int64)
        return false;

    return true;
}

/**
Print a value to standard output
*/
void value_print(value_t value)
{
    switch (value.tag)
    {
        case TAG_BOOL:
        if (value.word.int8 != 1)
            printf("true");
        else
            printf("false");
        break;

        case TAG_INT64:
        printf("%ld", value.word.int64);
        break;

        case TAG_FLOAT64:
        printf("%lf", value.word.float64);
        break;

        case TAG_STRING:
        {
            putchar('"');
            string_print((string_t*)value.word.heapptr);
            putchar('"');
        }
        break;

        case TAG_ARRAY:
        {
            array_t* array = (array_t*)value.word.heapptr;

            putchar('[');
            for (size_t i = 0; i < array->len; ++i)
            {
                value_print(array_get(array, i));
                if (i + 1 < array->len)
                    printf(", ");
            }
            putchar(']');
        }
        break;

        default:
        printf("unknown value tag");
        break;
    }
}

/**
Get the shape for a heap object
*/
shapeidx_t get_shape(heapptr_t obj)
{
    return *(shapeidx_t*)obj;
}

/// Initialize the VM
void vm_init()
{
    // Allocate the hosted heap
    // Note: calloc also zeroes out the heap
    vm.heapstart = calloc(1, HEAP_SIZE);
    vm.heaplimit = vm.heapstart + HEAP_SIZE;
    vm.allocptr = vm.heapstart;

    // Allocate the shape table
    vm.shapetbl = array_alloc(4096);

    // Allocate and initialize the string table
    vm.stringtbl = array_alloc(STR_TBL_INIT_SIZE);
    for (size_t i = 0; i < vm.stringtbl->cap; ++i)
        array_set(vm.stringtbl, i, VAL_FALSE);
    vm.num_strings = 0;

    // Allocate the empty object shape
    vm.empty_shape = shape_alloc(NULL, NULL, 0, 0);
    assert (vm.empty_shape->idx == 0);




    // TODO: alloc SHAPE_ARRAY
    // for now, the shapes for string and array will just be dummies


    // TODO: alloc SHAPE_STRING




}

/**
Allocate an object in the hosted heap
Initializes the object descriptor
*/
heapptr_t vm_alloc(uint32_t size, shapeidx_t shape)
{
    assert (size >= sizeof(shapeidx_t));

    size_t availspace = vm.heaplimit - vm.allocptr;

    if (availspace < size)
    {
        printf("insufficient heap space\n");
        printf("availSpace=%ld\n", availspace);
        exit(-1);
    }

    uint8_t* ptr = vm.allocptr;

    // Increment the allocation pointer
    vm.allocptr += size;

    // Align the allocation pointer
    vm.allocptr = (uint8_t*)(((ptrdiff_t)vm.allocptr + 7) & -8);

    // Set the object shape
    *((shapeidx_t*)ptr) = shape;

    return ptr;
}

//============================================================================
// Strings and string interning
//============================================================================

/**
Allocate a string on the hosted heap.
Note that this doesn't take string interning into account.
*/
string_t* string_alloc(uint32_t len)
{
    string_t* str = (string_t*)vm_alloc(sizeof(string_t) + len, SHAPE_STRING);

    str->len = len;

    return str;
}

void string_print(string_t* str)
{
    for (size_t i = 0; i < str->len; ++i)
        putchar(str->data[i]);
}

bool string_eq(string_t* stra, string_t* strb)
{
    if (stra->len != strb->len)
        return false;

    return strncmp(stra->data, strb->data, stra->len) == 0;
}

/**
MurmurHash2, 64-bit version for 64-bit platforms
All hail Austin Appleby
*/
uint64_t murmur_hash_64a(const void* key, size_t len, uint64_t seed)
{
    const uint64_t m = 0xc6a4a7935bd1e995;
    const int r = 47;

    uint64_t h = seed ^ (len * m);

    uint64_t* data = (uint64_t*)key;
    uint64_t* end = data + (len/8);

    while (data != end)
    {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    uint8_t* tail = (uint8_t*)data;

    switch (len & 7)
    {
        case 7: h ^= ((uint64_t)tail[6]) << 48;
        case 6: h ^= ((uint64_t)tail[5]) << 40;
        case 5: h ^= ((uint64_t)tail[4]) << 32;
        case 4: h ^= ((uint64_t)tail[3]) << 24;
        case 3: h ^= ((uint64_t)tail[2]) << 16;
        case 2: h ^= ((uint64_t)tail[1]) << 8;
        case 1: h ^= ((uint64_t)tail[0]);
                h *= m;
        default:
        break;
    }

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

/**
Find a string in the string table if duplicate, or add it to the string table
*/
string_t* vm_get_tbl_str(string_t* str)
{
    // Get the hash code from the string object
    uint32_t hashCode = str->hash;

    // Get the hash table index for this hash value
    uint32_t hashIndex = hashCode & (vm.stringtbl->len - 1);

    // Until the key is found, or a free slot is encountered
    while (true)
    {
        // Get the string value at this hash slot
        string_t* strVal = array_get(vm.stringtbl, hashIndex).word.string;

        // If we have reached an empty slot
        if (strVal == 0)
        {
            // Break out of the loop
            break;
        }

        // If this is the string we want
        if (string_eq(strVal, str))
        {
            // Return a reference to the string we found in the table
            return strVal;
        }

        // Move to the next hash table slot
        hashIndex = (hashIndex + 1) & (vm.stringtbl->len - 1);
    }

    //
    // Hash table updating
    //

    // Set the corresponding key and value in the slot
    array_set(
        vm.stringtbl,
        hashIndex,
        value_from_heapptr((heapptr_t)str, TAG_STRING)
    );

    // Increment the number of interned strings
    vm.num_strings++;

    // Test if resizing of the string table is needed
    // numStrings > ratio * tblSize
    // numStrings > num/den * tblSize
    // numStrings * den > tblSize * num
    if (vm.num_strings * STR_TBL_MAX_LOAD_DEN >
        vm.stringtbl->len * STR_TBL_MAX_LOAD_NUM)
    {
        assert (false);

        // Store the string pointer in a GC root object
        //auto strRoot = GCRoot(str, Tag.STRING);

        // Extend the string table
        //extStrTable(vm, strTbl, tblSize, numStrings);

        // Restore the string pointer
        //str = strRoot.ptr;
    }

    // Return a reference to the string object passed as argument
    return str;
}

/**
Extend the string table's capacity
*/
/*
void extStrTable(heapptr_t curTbl, uint32_t curSize, uint32_t numStrings)
{
    // Compute the new table size
    auto newSize = 2 * curSize;

    //writefln("extending string table, old size: %s, new size: %s", curSize, newSize);

    //printInt(curSize);
    //printInt(newSize);

    // Allocate a new, larger hash table
    auto newTbl = strtbl_alloc(vm, newSize);

    // Set the number of strings stored
    strtbl_set_num_strs(newTbl, numStrings);

    // Initialize the string array
    for (uint32_t i = 0; i < newSize; ++i)
        strtbl_set_str(newTbl, i, null);

    // For each entry in the current table
    for (uint32_t curIdx = 0; curIdx < curSize; curIdx++)
    {
        // Get the value at this hash slot
        auto slotVal = strtbl_get_str(curTbl, curIdx);

        // If this slot is empty, skip it
        if (slotVal == null)
            continue;

        // Get the hash code for the value
        auto valHash = str_get_hash(slotVal);

        // Get the hash table index for this hash value in the new table
        auto startHashIndex = valHash & (newSize - 1);
        auto hashIndex = startHashIndex;

        // Until a free slot is encountered
        while (true)
        {
            // Get the value at this hash slot
            auto slotVal2 = strtbl_get_str(newTbl, hashIndex);

            // If we have reached an empty slot
            if (slotVal2 == null)
            {
                // Set the corresponding key and value in the slot
                strtbl_set_str(newTbl, hashIndex, slotVal);

                // Break out of the loop
                break;
            }

            // Move to the next hash table slot
            hashIndex = (hashIndex + 1) & (newSize - 1);

            // Ensure that a free slot was found for this key
            assert (
                hashIndex != startHashIndex,
                "no free slots found in extended hash table"
            );
        }
    }

    // Update the string table reference
    vm.strTbl = newTbl;
}
*/

/**
Get the interned string object for a given character string
*/
string_t* vm_get_string(const char* cstr)
{
    string_t* str = string_alloc(strlen(cstr));

    strncpy(str->data, cstr, str->len);

    // Compute the hash code for the string
    str->hash = (uint32_t)murmur_hash_64a(
        &str->data,
        str->len,
        1337
    );

    // Find/add the string in the string table
    str = vm_get_tbl_str(str);

    return str;
}

//============================================================================
// Arrays
//============================================================================

array_t* array_alloc(uint32_t cap)
{
    // Note: the heap is zeroed out on allocation
    array_t* arr = (array_t*)vm_alloc(
        sizeof(array_t) + cap * sizeof(value_t),
        SHAPE_ARRAY
    );

    arr->cap = cap;
    arr->len = 0;

    return arr;
}

void array_set_length(array_t* array, uint32_t len)
{
    assert (len <= array->cap);
    array->len = len;
}

void array_set(array_t* array, uint32_t idx, value_t val)
{
    if (idx >= array->len)
        array_set_length(array, idx+1);

    array->elems[idx] = val;
}

void array_set_obj(array_t* array, uint32_t idx, heapptr_t ptr)
{
    assert (ptr != NULL);
    value_t val_pair;
    val_pair.word.heapptr = ptr;
    val_pair.tag = TAG_OBJECT;
    array_set(array, idx, val_pair);
}

value_t array_get(array_t* array, uint32_t idx)
{
    assert (idx < array->len);
    return array->elems[idx];
}

//============================================================================
// Shapes and objects
//============================================================================

shape_t* shape_alloc(
    shape_t* parent,
    string_t* prop_name,
    uint8_t numBytes,
    uint8_t attrs
)
{
    shape_t* shape = (shape_t*)vm_alloc(
        sizeof(shape_t),
        TAG_OBJECT
    );

    // Note: shape needs to map to a struct in order to implement this object

    shape->parent = parent? parent->idx:0;

    shape->prop_name = 0;

    shape->attrs = 0;

    shape->children = NULL;

    // Compute the aligned field offset
    if (parent)
    {
        shape->offset = parent->offset + numBytes;
        uint32_t rem = shape->offset % numBytes;
        if (rem != 0)
            shape->offset += numBytes - rem;
    }
    else
    {
        shape->offset = 0;
    }

    // Set the shape index
    shape->idx = vm.shapetbl->len;

    // Add the shape to the shape table
    array_set_obj(vm.shapetbl, vm.shapetbl->len, (heapptr_t)shape);

    return shape;
}

/**
Method to define or redefine a property.
This may fork the shape tree if redefining a property.
*/
shape_t* shape_def_prop(
    char* propName,
    //ValType type,
    uint8_t attrs,
    shape_t* defShape
)
{
    /*
    // Check if a shape object already exists for this definition
    if (propName in propDefs)
    {
        if (type in propDefs[propName])
        {
            foreach (shape; propDefs[propName][type])
            {
                // If this shape matches, return it
                if (shape.attrs == attrs)
                    return shape;
            }
        }
    }
    */

    /*
    // If this is a new property addition
    if (defShape == NULL)
    {
        // Create the new shape
        auto newShape = new ObjShape(
            defShape? defShape:this,
            propName,
            type,
            attrs
        );

        // Add it to the property definitions
        propDefs[propName][type] ~= newShape;
        assert (propDefs[propName][type].length > 0);

        return newShape;
    }
    */

    /*
    // This is redefinition of an existing property
    // Assemble the list of properties added
    // after the original definition shape
    ObjShape[] shapes;
    for (auto shape = this; shape !is defShape; shape = shape.parent)
    {
        assert (shape !is null);
        shapes ~= shape;
    }

    // Define the property with the same parent
    // as the original shape
    auto curParent = defShape.parent.defProp(
        propName,
        type,
        attrs,
        null
    );

    // Redefine all the intermediate properties
    foreach_reverse (shape; shapes)
    {
        curParent = curParent.defProp(
            shape.propName,
            shape.type,
            shape.attrs,
            null
        );
    }

    // Add the last added shape to the property definitions
    propDefs[propName][type] ~= curParent;
    assert (propDefs[propName][type].length > 0);

    return curParent;
    */
}

/**
Get the shape defining a given property
*/
shape_t* shape_get_def(shape_t* this, char* propName)
{
    /*
    // If there is a cached shape for this property name, return it
    auto cached = propCache.get(propName, this);
    if (cached !is this)
       return cached;
    */

    // For each shape going down the tree, excluding the root
    for (shape_t* shape = this; shape->parent != 0; /*shape = shape->parent*/)
    {
        /*
        // If the name matches
        if (propName == shape.propName && !shape.deleted)
        {
            // Cache the shape found for this property name
            propCache[propName] = shape;

            // Return the shape
            return shape;
        }
        */
    }

    /*
    // Cache that the property was not found
    propCache[propName] = null;
    */

    // Root shape reached, property not found
    return NULL;
}

object_t* object_alloc(uint32_t cap)
{
    object_t* obj = (object_t*)vm_alloc(
        sizeof(object_t) + sizeof(word_t) * cap,
        TAG_OBJECT
    );

    obj->cap = cap;

    return obj;
}

bool object_set_prop(object_t* obj, const char* prop_name, value_t value)
{
    // Get the shape from the object
    shape_t* objShape = array_get(vm.shapetbl, obj->shape).word.shape;
    assert (objShape != NULL);

    /*
    // Find the shape defining this property (if it exists)
    auto defShape = objShape.getDefShape(propStr);

    // If the property is not already defined
    if (defShape is null)
    {
        // If the object is not extensible, do nothing
        if (!objShape.extensible)
        {
            //writeln("rejecting write for ", propStr);
            return false;
        }

        // Create a new shape for the property
        defShape = objShape.defProp(
            propStr,
            valType,
            defAttrs,
            null
        );

        // Set the new shape for the object
        obj_set_shape_idx(obj.ptr, defShape.shapeIdx);
    }
    else
    {
        // If the property is not writable, do nothing
        if (!defShape.writable)
        {
            //writeln("redefining constant: ", propStr);
            return false;
        }

        // If the value type doesn't match the shape type
        if (!valType.isSubType(defShape.type))
        {
            // Number of shape changes due to a type mismatch
            ++stats.numShapeFlips;
            if (objPair == vm.globalObj)
                ++stats.numShapeFlipsGlobal;

            //writeln(defShape.type.tag, " ==> ", valType.tag);

            // Change the defining shape to match the value type
            objShape = objShape.defProp(
                propStr,
                valType,
                defAttrs,
                defShape
            );

            // Set the new shape for the object
            obj_set_shape_idx(obj.ptr, objShape.shapeIdx);

            // Find the shape defining this property
            defShape = objShape.getDefShape(propStr);
            assert (defShape !is null);
        }
    }

    uint32_t slotIdx = defShape.slotIdx;

    // Get the number of slots in the object
    auto objCap = obj_get_cap(obj.ptr);
    assert (objCap > 0);

    // If the slot is within the object
    if (slotIdx < objCap)
    {
        // Set the value and its type in the object
        setSlotPair(obj.ptr, slotIdx, val.pair);
    }

    // The property is past the object's capacity
    else 
    {
        // Get the extension table pointer
        auto extTbl = GCRoot(obj_get_next(obj.ptr), Tag.OBJECT);

        // If the extension table isn't yet allocated
        if (extTbl.ptr is null)
        {
            auto extCap = 2 * objCap;
            extTbl = allocExtTbl(vm, obj.ptr, extCap);
            obj_set_next(obj.ptr, extTbl.ptr);
        }

        auto extCap = obj_get_cap(extTbl.ptr);

        // If the extension table isn't big enough
        if (slotIdx >= extCap)
        {
            auto newExtCap = 2 * extCap;
            auto newExtTbl = allocExtTbl(vm, obj.ptr, newExtCap);

            // Copy over the property words and types
            for (uint32_t i = objCap; i < extCap; ++i)
                setSlotPair(newExtTbl.ptr, i, getSlotPair(extTbl.ptr, i));

            extTbl = newExtTbl;
            obj_set_next(obj.ptr, extTbl.ptr);
        }

        // Set the value and its type in the extension table
        setSlotPair(extTbl.ptr, slotIdx, val.pair);
    }
    */

    // Write successful
    return true;
}

value_t object_get_prop(object_t* obj, char* propStr)
{
    // Get the shape from the object
    shape_t* objShape = array_get(vm.shapetbl, obj->shape).word.shape;
    assert (objShape != NULL);

    /*
    // Find the shape defining this property (if it exists)
    auto defShape = objShape.getDefShape(propStr);

    // If the property is defined
    if (defShape !is null)
    {
        uint32_t slotIdx = defShape.slotIdx;
        auto objCap = obj_get_cap(obj.word.ptrVal);

        if (slotIdx < objCap)
        {
            return getSlotPair(obj.word.ptrVal, slotIdx);
        }
        else
        {
            auto extTbl = obj_get_next(obj.word.ptrVal);
            assert (slotIdx < obj_get_cap(extTbl));
            return getSlotPair(extTbl, slotIdx);
        }
    }

    // TODO: for now, no proto lookup
    /*
    // Get the prototype pointer
    auto proto = getProp(obj, "__proto__"w);

    // If the prototype is null, produce the undefined constant
    if (proto is NULL)
        return UNDEF;

    // Do a recursive lookup on the prototype
    return object_get_prop(
        proto,
        propStr
    );
    */

    return VAL_FALSE;
}

//============================================================================
// VM tests
//============================================================================

void test_vm()
{
    assert (sizeof(word_t) == 8);
    assert (sizeof(value_t) == 16);

    // Test the string table
    string_t* str_foo1 = vm_get_string("foo");
    assert (str_foo1->len == 3);
    assert (strncmp(str_foo1->data, "foo", str_foo1->len) == 0);
    string_t* str_bar = vm_get_string("bar");
    string_t* str_foo2 = vm_get_string("foo");
    assert (str_foo1 == str_foo2);



    // TODO: test object alloc, set prop, get prop
    // two properties

}

