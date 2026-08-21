#ifndef MORROW_EDITOR_EVENTS_H_
#define MORROW_EDITOR_EVENTS_H_

#include "core/Observable.h"
#include "scene/SceneEditorModel.h"

namespace morrow::editor
{
class AssetDatabase;

/**
 * @brief Editor-local synchronous events owned by one EditorShell instance.
 *
 * Runtime code must not depend on this type.
 */
struct EditorEvents {
    Observable<const SelectionState&> onSelectionChanged;
    Observable<> onRuntimeRebuilt;
    Observable<const AssetDatabase&> onAssetDatabaseChanged;
};
}

#endif // MORROW_EDITOR_EVENTS_H_
