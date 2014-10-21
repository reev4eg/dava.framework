//
//  ComponentIndices.cpp
//  Framework
//
//  Created by Dmitry Belsky on 7.10.14.
//
//

#include "ComponentIndices.h"

#include "Entity.h"


namespace DAVA {
    ComponentIndices *ComponentIndices::workCopy = NULL;
    Vector<ComponentIndices*> *ComponentIndices::indicesInstances = NULL;

    void ComponentIndices::Clean() {
        for (int i = 0; i < Component::COMPONENT_COUNT; i++) {
            indices[i] = -1;
            count[i] = 0;
        }
    }
    
    ComponentIndices *ComponentIndices::makeFromWorkCopy() {
        // try to find equal indices
        for (auto it = indicesInstances->begin(); it != indicesInstances->end(); ++it)
        {
            ComponentIndices *test = *it;
            bool eq = true;
            for (int i = 0; i < Component::COMPONENT_COUNT; i++) {
                if (test->indices[i] != workCopy->indices[i] || test->count[i] != workCopy->count[i])
                {
                    eq = false;
                    break;
                }
            }
            if (eq)
            {
                return test;
            }
        }
        
        // make new instance
        ComponentIndices *indices = new ComponentIndices();
        for (int i = 0; i < Component::COMPONENT_COUNT; i++) {
            indices->indices[i] = workCopy->indices[i];
            indices->count[i] = workCopy->count[i];
        }
        indicesInstances->push_back(indices);
        Logger::Debug("!!!! INDICES INSTANCES COUNT: %d", indicesInstances->size());
        return indices;
    }

    void ComponentIndices::Init()
    {
        workCopy = new ComponentIndices();
        indicesInstances = new Vector<ComponentIndices*>;
    }

}

