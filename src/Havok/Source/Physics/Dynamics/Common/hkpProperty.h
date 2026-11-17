#pragma once

// The property keys the SDK reserves for its own use (hkpWorldObject::getProperty and its siblings take them).
// The id a display object of the entity goes by in the visual debugger, in place of its collidable's address.
#define HK_PROPERTY_DEBUG_DISPLAY_ID 0x1134
// The colour the entity's display objects are drawn in (0: the viewer's automatic colour).
#define HK_PROPERTY_DEBUG_DISPLAY_COLOR 0x1130
// An hkGeometry shown for the entity in place of its shape's; the shape display viewer removes the property and its
// display object owns the geometry.
#define HK_PROPERTY_OVERRIDE_DEBUG_DISPLAY_GEOMETRY 0x1131
// The shape the entity is shown with in place of its collidable's.
#define HK_PROPERTY_DISPLAY_SHAPE 0x1135
// As HK_PROPERTY_OVERRIDE_DEBUG_DISPLAY_GEOMETRY, with the geometry left to its owner.
#define HK_PROPERTY_OVERRIDE_DEBUG_DISPLAY_GEOMETRY_NO_DELETE 0x1136
