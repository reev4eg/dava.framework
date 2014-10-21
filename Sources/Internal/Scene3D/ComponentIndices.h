//
//  ComponentIndices.h
//  Framework
//
//  Created by Dmitry Belsky on 7.10.14.
//
//

#ifndef __Framework__ComponentIndices__
#define __Framework__ComponentIndices__

#include "Entity/Component.h"

namespace DAVA {

    class ComponentIndices {
    public:
        int indices[Component::COMPONENT_COUNT];
        int count[Component::COMPONENT_COUNT];
        
        void Clean();

        static ComponentIndices *workCopy;
        static ComponentIndices *makeFromWorkCopy();
        static Vector<ComponentIndices*> *indicesInstances;
        
        static void Init();
    };
    
}

#endif /* defined(__Framework__ComponentIndices__) */
