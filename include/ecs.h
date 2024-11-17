#pragma once

#include "serialize.h"
#include <climits>

// #define ECS_DEBUG_OFF

using entity = uint32_t; /** @brief Alias for a 32-bit unsigned value.*/

namespace object
{
    struct ecs
    {
        private:
            struct SystemFunction;

        public:
            /**
             * @brief Holds a list of `Entity`'s based on their components.
             * 
             * @details Allows the user to iterate over `Entity`'s with common components in user-defined 
             *          functions, holds a static instance of the `System`'s parent data type.
             */
            struct system
            {
                int32_t priority = 0;

                bool initialized;                                       /** @brief Tracks whether the `System` has been fully initialized.*/
                std::vector<uint8_t> instance;                          /** @brief The raw static instance data.*/
                
                std::vector<SystemFunction> functions; /** @brief Various user-defined or default functions that can be run on the `System`.*/
                std::vector<SystemFunction> functionBuffer;


                static size_t length(const system& data)
                {
                    return 
                        object::length(data.priority) + 
                        object::length(data.initialized) +
                        object::length(data.instance);
                }

                static size_t serialize(const system& value, std::vector<uint8_t>& stream, size_t index)
                {
                    size_t count = 0;

                    count += object::serialize(value.priority, stream, index + count);
                    count += object::serialize(value.initialized, stream, index + count);
                    count += object::serialize(value.instance, stream, index + count);

                    return count;
                }

                static system deserialize(std::vector<uint8_t>& stream, size_t index)
                {
                    system result = system();
                    size_t count = 0;

                    result.priority = object::deserialize<int32_t>(stream, index + count);
                    count += object::length(result.priority);

                    result.initialized = object::deserialize<bool>(stream, index + count);
                    count += object::length(result.initialized);

                    result.instance = object::deserialize<std::vector<uint8_t>>(stream, index + count);
                    count += object::length(result.instance);

                    return result;
                }


                /**
                 * @brief Constructor for struct 'System'.
                 * 
                 * @param space The size of `System`'s parent data type needed for memory allocation.
                 */
                system() {}

                system(uint8_t functionSize)
                {
                    functions = std::vector<SystemFunction>(functionSize, SystemFunction());
                    functionBuffer = functions;
                    initialized = false;
                }

                /**
                 * @brief Finishes initializing this `System`.
                 * 
                 * @details The `System`'s static instance, its constructor, and its destructor are fully 
                 *          initialized and set.
                 * 
                 * @tparam T The type of `System`'s static instance.
                 * @param instance The initial data of `System's` static instance.
                 */
                template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
                void initialize(const T& inst)
                {
                    initialized = true;
                    instance.resize(sizeof(T));
                    object::serialize<T>(inst, instance, 0);
                }

                template<typename T, typename = std::enable_if_t<!std::is_trivially_copyable<T>::value>>
                void initialize(const T inst, bool temp = false)
                {
                    initialized = true;
                    instance.resize(object::length(inst) + sizeof(size_t));
                    object::serialize<T>(inst, instance, 0);
                }

                /**
                 * @brief Determines whether the current `System` has been fully initialized.
                 * 
                 * @return The initialization state of this `System`.
                 */
                bool& isInitialized()
                {
                    return initialized;
                }

                /**
                 * @brief Creates a new usable space for a user-defined function.
                 */
                void createFunction()
                {
                    functions.push_back(SystemFunction());
                    functionBuffer.push_back(SystemFunction());
                }


                /**
                 * @brief Sets a user-defined function pointer at a certain index.
                 * 
                 * @param index The index where the function pointer will be placed.
                 * @param function The new function pointer that will be stored.
                 */
                void setFunction(uint8_t index, void (*function)(ecs&, system&, void *))
                {
                    functions[index].func = function;
                }


                /**
                 * @brief Runs the user-defined or default function at a certain index.
                 * 
                 * @param index The index where the function pointer will be placed.
                 */
                void runFunction(ecs& container, uint8_t index, void *data)
                {
                    functions[index].func(container, *this, data);
                }


                void setActive(uint8_t index, bool state)
                {
                    bool& current = functions[index].active;
                    if(state && !current)
                    {
                        auto temp = functions[index].func;
                        functions[index].func = functionBuffer[index].func;
                        functionBuffer[index].func = temp;

                        current = true;
                    }
                    else if(!state && current)
                    {
                        auto temp = functions[index].func;
                        functions[index].func = functionBuffer[index].func;
                        functionBuffer[index].func = temp;

                        current = false;
                    }
                }

                void setActive(bool state)
                {
                    for(size_t i=0; i<functions.size(); i++)
                    {
                        bool& current = functions[i].active;
                        if(state && !current)
                        {
                            auto temp = functions[i].func;
                            functions[i].func = functionBuffer[i].func;
                            functionBuffer[i].func = temp;

                            current = true;
                        }
                        else if(!state && current)
                        {
                            auto temp = functions[i].func;
                            functions[i].func = functionBuffer[i].func;
                            functionBuffer[i].func = temp;

                            current = false;
                        }
                    }
                }

                void toggle()
                {
                    for(size_t i=0 ; i<functions.size(); i++)
                    {
                        bool& current = functions[i].active;
                        auto temp = functions[i].func;
                        functions[i].func = functionBuffer[i].func;
                        functionBuffer[i].func = temp;
                        current = !current;
                    }
                }

                bool functionActive(uint8_t index)
                {
                    return functions[index].active;
                }

                /**
                 * @brief Converts this `System`'s static instance from unsigned 8-bit data into its original type.
                 * 
                 * @tparam T The original data type of this `System`'s static instance.
                 * @return The 8-bit data transformed into a usable type.
                 */
                template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
                T& getInstance()
                {
                    return object::deserialize<T>(instance, 0);
                }

                template<typename T, typename = std::enable_if_t<!std::is_trivially_copyable<T>::value>>
                T getInstance()
                {
                    return object::deserialize<T>(instance, 0);
                }

                template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
                void pushInstance(const T& inst)
                {
                    instance.resize(sizeof(T));
                    object::serialize<T>(inst, instance, 0);
                }

                template<typename T, typename = std::enable_if_t<!std::is_trivially_copyable<T>::value>>
                void pushInstance(const T inst, bool temp = false)
                {
                    instance.resize(object::length(inst) + sizeof(size_t));
                    object::serialize<T>(inst, instance, 0);
                }
            };
            

        static size_t length(const object::ecs& data)
        {
            return
                object::length(data.entityManager) +
                object::length(data.componentManager) +
                object::length(data.systemManager);
        }

        static size_t serialize(const object::ecs& value, std::vector<uint8_t>& stream, size_t index)
        {
            size_t count = 0;

            count += object::serialize(value.entityManager, stream, index + count);
            count += object::serialize(value.componentManager, stream, index + count);
            count += object::serialize(value.systemManager, stream, index + count);

            return count;
        }

        static object::ecs deserialize(std::vector<uint8_t>& stream, size_t index)
        {
            object::ecs result = object::ecs();
            size_t count = 0;

            result.entityManager = object::deserialize<object::ecs::EntityManager>(stream, index + count);
            count += object::length(result.entityManager);

            result.componentManager = object::deserialize<object::ecs::ComponentManager>(stream, index + count);
            count += object::length(result.componentManager);

            result.systemManager = object::deserialize<object::ecs::SystemManager>(stream, index + count);
            count += object::length(result.systemManager);

            return result;
        }


        ecs()
        {
            entity totalEntities = entityManager.totalEntityCount();
            componentManager = ComponentManager(totalEntities);
            systemManager = SystemManager(totalEntities);
        }

        entity createEntity()
        {
            systemManager.update(entityManager.totalEntityCount());
            systemManager.addEntity();
            
            componentManager.update(entityManager.totalEntityCount());
            componentManager.addID();
            entity e = entityManager.createEntity();

            addComponent<bool>(e, true);

            return e;
        }

        void removeEntity(entity e)
        {
            #ifndef ECS_DEBUG_OFF
                if(!entityManager.contains(e))
                {
                    error = 6;
                    return;
                }
            #endif

            componentManager.removeID(e);
            systemManager.extractEntity(e, entityManager.getBitmap(e));
            entityManager.removeEntity(e);
        }

        entity numberOfEntities()
        {
            return entityManager.totalEntityCount();
        }

        bool active(entity e)
        {
            return getComponent<bool>(e);
        }

        void setActive(entity e, bool newState)
        {
            #ifndef ECS_DEBUG_OFF
                if(!entityManager.contains(e))
                {
                    error = 6;
                    return;
                }
            #endif
            bool& state = getComponent<bool>(e);
            if(state == newState)
            {
                return;
            }

            state = newState;
            if(newState)
            {
                std::vector<bool>& bitmap = entityManager.getBitmap(e);

                for(uint32_t i=0; i<systemManager.stores.size(); i++)
                {
                    if(systemManager.bitmapMatches(i, bitmap))
                    {
                        systemManager.insertEntity(*this, e, i);
                    }
                }
            }
            else
            {
                systemManager.extractEntity(e, entityManager.getBitmap(e));
            }
        }

        entity clone()
        {
            entity clone = createEntity();
            
            return clone;
        }

        size_t numberOfComponents()
        {
            return ComponentManager::cidCount;
        }

        template <typename T>
        bool active(entity e)
        {
            #ifndef ECS_DEBUG_OFF
                if(!entityManager.contains(e))
                {
                    error = 6;
                    return false;
                }
            #endif

            return entityManager.getBitmap(e)[ComponentType<T>::id];
        }

        template <typename T>
        void setActive(entity e, bool newState)
        {
            #ifndef ECS_DEBUG_OFF
                if(!entityManager.contains(e))
                {
                    error = 6;
                    return;
                }
            #endif

            uint32_t id = ComponentType<T>::id;
            std::vector<bool>& bitmap = entityManager.getBitmap(e);
            if(bitmap[id] == newState || !componentManager.containsComponent<T>(e))
            {
                return;
            }

            if(!bitmap[id])
                bitmap[id] = true;

            if(newState)
            {
                for(int i=0; i<systemManager.stores.size(); i++)
                {
                    if(systemManager.bitmapMatches(i, bitmap, id))
                    {
                        systemManager.insertEntity(*this, e, i);
                    }
                }
            }
            else
            {
                systemManager.componentRemoved(e, id, bitmap);
            }

            if(bitmap[id])
                bitmap[id] = false;
        }

        template <typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, int> = 0>
        T& addComponent(entity e, const T& component = T())
        {
            uint32_t id = ComponentType<T>::id;

            #ifndef ECS_DEBUG_OFF
                if(!entityManager.contains(e))
                {
                    error = 6;
                    return componentManager.getComponent<T>(-1, id);
                }
            #endif

            T& result = componentManager.addComponent<T>(e, id, component);
            addComponentConfiguration(e, id);    
            return result;
        }

        template <typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, int> = 0>
        T addComponent(entity e, const T& component = T())
        {
            uint32_t id = ComponentType<T>::id;

            #ifndef ECS_DEBUG_OFF
                if(!entityManager.contains(e))
                {
                    error = 6;
                    return componentManager.getComponent<T>(-1, id);
                }
            #endif

            T result = componentManager.addComponent<T>(e, id, component);
            addComponentConfiguration(e, id);
            
            return result;
        }

        template <typename T>
        void shareComponent(entity e, entity share)
        {
            #ifndef ECS_DEBUG_OFF
                if(!entityManager.contains(e))
                {
                    error = 6;
                    return;
                }
            #endif

            uint32_t id = ComponentType<T>::id;
            componentManager.share<T>(e, share, id);
            addComponentConfiguration(e, id);
        }

        template<typename T>
        void setComponent(entity e, const T& update)
        {
            #ifndef ECS_DEBUG_OFF
                if(!entityManager.contains(e))
                {
                    e = -1;
                    error = 6;
                }
            #endif

            componentManager.setComponent<T>(e, ComponentType<T>::id, update);
        }

        template <typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, int> = 0>
        T& getComponent(entity e)
        {
            #ifndef ECS_DEBUG_OFF
                if(!entityManager.contains(e))
                {
                    e = -1;
                    error = 6;
                }
            #endif

            return componentManager.getComponent<T>(e, ComponentType<T>::id);
        }

        template <typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, int> = 0>
        T getComponent(entity e)
        {
            #ifndef ECS_DEBUG_OFF
                if(!entityManager.contains(e))
                {
                    e = -1;
                    error = 6;
                }
            #endif

            return componentManager.getComponent<T>(e, ComponentType<T>::id);
        }

        template <typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, int> = 0>
        T& getComponentAt(entity e, size_t index)
        {
            #ifndef ECS_DEBUG_OFF
                if(!entityManager.contains(e))
                {
                    e = -1;
                    error = 6;
                }
            #endif

            return componentManager.getComponentAt<T>(e, ComponentType<T>::id, index);
        }

        template<typename T>
        size_t getCompressedIndex(entity e)
        {
            return componentManager.getCompressedIndex<T>(e, ComponentType<T>::id);
        }

        template <typename T>
        T& getDefaultComponent()
        {
            return componentManager.getDefaultComponent<T>(ComponentType<T>::id);
        }

        template<typename T>
        bool containsComponent(entity e)
        {
            return componentManager.containsComponent<T>(e, ComponentType<T>::id);
        }

        template <typename T>
        T removeComponent(entity e)
        {
            #ifndef ECS_DEBUG_OFF
                if(!entityManager.contains(e))
                {
                    error = 6;
                    return T();
                }
            #endif

            uint32_t id = ComponentType<T>::id;
            systemManager.componentRemoved(e, id, entityManager.getBitmap(e));
            entityManager.setComponentBit(e, id, false);
                
            return componentManager.removeComponent<T>(e);
        }

        uint8_t createSystemFunction()
        {
            return systemManager.createSystemFunction();
        }

        template <typename T>
        std::vector<entity>& entities()
        {
            return systemManager.entities<T>();
        }

        template<typename T, typename... Args>
        system& createSystem(const T& instance = T(), int32_t priority = 0)
        {
            uint32_t& id = SystemType<T>::id;

            systemManager.update(entityManager.totalEntityCount());
            system& result = systemManager.createSystem<T>(instance, priority, id);

            systemManager.addRequirements<T, Args...>();
            
            entity totalEntities = entityManager.totalEntityCount();
            for(entity i=0; i<totalEntities; i++)
            {
                if(entityManager.entityActive(i))
                {
                    std::vector<bool> bitmap = entityManager.getBitmap(i);
                    if(systemManager.bitmapMatches(id, bitmap))
                    {
                        systemManager.insertEntity(*this, i, id);
                    }
                }
            }

            return result;
        }

        template <typename T>
        system& getSystem()
        {
            return systemManager.getSystem(SystemType<T>::id);
        }

        template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
        T& getInstance()
        {
            return getSystem<T>().template getInstance<T>();
        }

        template<typename T, typename = std::enable_if_t<!std::is_trivially_copyable<T>::value>>
        T getInstance()
        {
            return getSystem<T>().template getInstance<T>();
        }

        template<typename T>
        void setInsertion(void (*insert)(ecs&, entity, std::vector<entity>&, std::vector<size_t>&))
        {
            systemManager.setInsertion(SystemType<T>::id, insert);
        }

        template<typename T>
        std::vector<size_t>& getMapping()
        {
            return systemManager.getIndexMap<T>();
        }

        void run(uint8_t index, void *data = NULL)
        {
            #ifndef ECS_DEBUG_OFF
                if(!systemManager.containsFunction(index))
                {
                    error = 5;
                    return;
                }
            #endif
            systemManager.runFunction(*this, index, data);
        }

        template<typename T>
        void setFunctionActive(uint8_t index, bool state)
        {
            #ifndef ECS_DEBUG_OFF
                if(!systemManager.containsFunction(index))
                {
                    error = 5;
                    return;
                }
            #endif
            uint32_t id = SystemType<T>::id;
            systemManager.setActive(id, index, state);
        }

        template<typename T>
        void setFunctionsActive(bool state)
        {
            uint32_t id = SystemType<T>::id;
            systemManager.setActive(id, state);
        }

        template<typename T>
        void toggleFunction(uint8_t index)
        {
            uint32_t id = SystemType<T>::id;
            systemManager.toggleFunction(id, index);
        }

        template<typename T>
        void toggleFunctions()
        {
            uint32_t id = SystemType<T>::id;
            systemManager.toggleFunctions(id);
        }

        template<typename T>
        bool functionActive(uint8_t index)
        {
            uint32_t id = SystemType<T>::id;
            return systemManager.functionActive(id, index);
        }

        void clearEntities()
        {
            entityManager = EntityManager();
            componentManager = ComponentManager(0);
            systemManager.clearEntities();
        }

        uint32_t createSystemToggle()
        {
            return systemManager.createToggle();
        }

        void toggle(uint32_t index)
        {
            systemManager.toggle(index);
        }

        template<typename T>
        void addToToggle(uint32_t index, uint8_t func)
        {
            uint32_t id = SystemType<T>::id;
            systemManager.addToggle(index, id, func);
        }

        template<typename T>
        void addToToggle(uint32_t index)
        {
            uint32_t id = SystemType<T>::id;
            systemManager.addToggles(index, id);
        }

        //
        static uint16_t getError()
        {
            uint16_t err = error;
            error = 0;
            return err;
        }

        //
        static std::string parseError()
        {
            uint16_t err = getError();
            switch(err)
            {
                case 0:
                    return "SUCCESS";
                break;
                case 1:
                    return "ERROR :: Entity already contains component; Call to `addComponent` failed.";
                break;
                case 2:
                    return "ERROR :: Component could not be accessed from call to `getComponent`.";
                break;
                case 3:
                    return "ERROR :: Component was not present from call to `removeComponent`.";
                break;
                case 4:
                    return "ERROR :: Entity has been deleted; Call to `addComponent` failed.";
                break;
                case 5:
                    return "ERROR :: System does not exist.";
                break;
                case 6:
                    return "ERROR :: Entity does not exist.";
                break;
                case 7:
                    return "ERROR :: Archetypes not enabled, but an archetype-exclusive function was used.";
                break;
            }
            return "N/A.";
        }

        //
        template<typename T>
        static uint32_t systemID()
        {
            return SystemType<T>::id;
        }

        private:
            /**
             * @brief A manager for created and destroyed `Entity` variables.
             * 
             * @details Handles the creation and destruction of unique `Entity` values. Also tends to each 
             *          `Entity`'s component bitmap.
             */
            struct EntityManager
            {
                entity entityCount = 0;                          /** @brief The total number of active entities.*/
                std::vector<entity> removedEntities;             /** @brief A list of every entity that has been removed.*/
                std::vector<std::vector<bool>> componentBitmaps; /** @brief The component bitmaps corresponding to each entity.*/

                static size_t length(const object::ecs::EntityManager& data)
                {
                    return 
                        object::length(data.entityCount) + 
                        object::length(data.removedEntities) +
                        object::length(data.componentBitmaps);
                }

                static size_t serialize(const object::ecs::EntityManager& value, std::vector<uint8_t>& stream, size_t index)
                {
                    size_t count = 0;

                    count += object::serialize(value.entityCount, stream, index + count);
                    count += object::serialize(value.removedEntities, stream, index + count);
                    count += object::serialize(value.componentBitmaps, stream, index + count);

                    return count;
                }

                static object::ecs::EntityManager deserialize(std::vector<uint8_t>& stream, size_t index)
                {
                    object::ecs::EntityManager result = object::ecs::EntityManager();
                    size_t count = 0;

                    result.entityCount = object::deserialize<entity>(stream, index + count);
                    count += object::length(result.entityCount);

                    result.removedEntities = object::deserialize<std::vector<uint32_t>>(stream, index + count);
                    count += object::length(result.removedEntities);

                    result.componentBitmaps = object::deserialize<std::vector<std::vector<bool>>>(stream, index + count);
                    count += object::length(result.componentBitmaps);

                    return result;
                }


                EntityManager()
                {
                    entityCount = 0;
                    removedEntities = std::vector<entity>();
                    componentBitmaps = std::vector<std::vector<bool>>();
                }

                /**
                 * @brief Creates an 'Entity' with a unique value.
                 * 
                 * @details Either returns and Entity with an incremented value, or recycles an Entity that 
                 *          has been deleted. Also initializes the `Entity`'s bitmap.
                 * 
                 * @return Newly created `Entity` 
                 */
                entity createEntity()
                {
                    entity entity = entityCount++;

                    // recycles any entities that have been destroyed
                    if(!removedEntities.empty())
                    {
                        entity = removedEntities.back();
                        removedEntities.pop_back();
                    }
                    // creates a new bitmap if no entities can be recycled
                    else
                    {
                        componentBitmaps.push_back(std::vector<bool>(ComponentManager::cidCount + 1));
                    }
                    componentBitmaps[entity].back() = true;
                    return entity;
                }

                /**
                 * @brief Removes and recycles an 'Entity'
                 * 
                 * @details Stores the deleted `Entity` in an internal vector. Resets that `Entity`'s bitmap.
                 * 
                 * @param entity An `Entity` created by the `createEntity` function.
                 */
                void removeEntity(entity entity)
                {
                    // resets the bitmap to be recycled
                    componentBitmaps[entity] = std::vector<bool>(ComponentManager::cidCount + 1);
                    removedEntities.push_back(entity);
                    entityCount--;
                }

                /**
                 * @brief Sets the bit in the bitmap of a certain `Entity` at a defined index.
                 * 
                 * @param entity An `Entity` created by the `createEntity` function.
                 * @param index  The index of the bit to set.
                 * @param bit    The value the bit will be set to.
                 */
                void setComponentBit(entity entity, uint32_t index, bool bit)
                {
                    componentBitmaps[entity][index] = bit;
                }

                /**
                 * @brief Returns the bitmap of a certain `Entity`.
                 * 
                 * @param entity An `Entity` created by the `createEntity` function.
                 * @return The component bitmap attached to the given `Entity`.
                 */
                std::vector<bool>& getBitmap(entity entity)
                {
                    return componentBitmaps[entity];
                }

                /**
                 * @brief Determines whether the given `Entity` has been removed or is still active.
                 * 
                 * @param entity An `Entity` created by the `createEntity` function.
                 * @return The `active` state of an `Entity`.
                 */
                bool entityActive(entity entity) const
                {
                    return componentBitmaps[entity].back();
                }


                bool contains(entity entity) const
                {
                    return totalEntityCount() > entity;
                }

                /**
                 * @return The number of active entities.
                 */
                entity activeEntityCount() const
                {
                    return entityCount;
                }

                /**
                 * @return The total number of entities (active and removed).
                 */
                entity totalEntityCount() const
                {
                    return entityCount + removedEntities.size();
                }
            };

            /**
             * @brief A component pool that holds various data of a certain type.
             * 
             * @details Although templates are not explicitly used for the struct, a `ComponentArray` can only hold a single type.
             *          A vector of 8-bit unsigned values is used to hold this information.
             *          Data can easily be transformed into and out of this format.
             */
            struct ComponentArray
            {
                size_t count = 0;
                size_t componentSize;            /** @brief The size of the component the `components` vector holds.*/
                std::vector<uint8_t> components; /** @brief An vector of 8-bit unsigned values that holds all the component data.*/


                static size_t length(const ComponentArray& data)
                {
                    return 
                        object::length(data.componentSize) + 
                        object::length(data.components);
                }

                static size_t serialize(const ComponentArray& value, std::vector<uint8_t>& stream, size_t index)
                {
                    size_t count = 0;

                    count += object::serialize(value.componentSize, stream, index + count);
                    count += object::serialize(value.components, stream, index + count);

                    return count;
                }

                static ComponentArray deserialize(std::vector<uint8_t>& stream, size_t index)
                {
                    ComponentArray result = ComponentArray();
                    size_t count = 0;

                    result.componentSize = object::deserialize<size_t>(stream, index + count);
                    count += object::length(result.componentSize);

                    result.components = object::deserialize<std::vector<uint8_t>>(stream, index + count);
                    count += object::length(result.components);

                    return result;
                }


                ComponentArray() {}

                ComponentArray(size_t size, bool complexity)
                {
                    components.resize(sizeof(bool) + size);
                    object::serialize<bool>(!complexity, components, 0);
                    componentSize = size;
                }

                size_t overwrite(size_t index)
                {
                    size_t offset;
                    if(complex())
                    {
                        offset = object::deserialize<size_t>(components, index) + sizeof(size_t);
                    }
                    else
                    {
                        offset = componentSize;
                    }

                    std::memcpy(&components[index], &components[index + offset], components.size() - (index + offset));
                    components.resize(components.size() - offset);

                    return offset;
                }

                bool complex()
                {
                    return object::deserialize<bool>(components, 0);
                }

                /**
                 * @brief Attaches data based off a provided `Entity`.
                 * 
                 * @details Defined after the definition of struct `ecs`.
                 *          Allows the user to access specific data using an `Entity` as a unique index.
                 *          Only single piece of data can be allocated per type per Entity.
                 * 
                 * @tparam T The type of component to be added to `entity`.
                 * @param entity An `Entity` made by the `createEntity` function. Used to index component data.
                 * @param component The component data to be linked to `entity`.
                 * @return Reference to the component in its component pool.
                 */
                template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
                T& addComponent(size_t& index, const T& component);


                template<typename T, typename = std::enable_if_t<!std::is_trivially_copyable<T>::value>>
                T addComponent(size_t& index, const T& component);

                /**
                 * @brief Returns data based off a provided `Entity`.
                 * 
                 * @details Defined after the definition of struct `ecs`.
                 *          Retrieves a piece of data of type T attached to the `entity` if such data exists.
                 * 
                 * @tparam T The type of component to be retrieved.
                 * @param entity An `Entity` made by the `createEntity` function. Used to index component data.
                 * @return Reference to the component in its component pool.
                 */
                template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
                T& getComponent(size_t index);

                template<typename T, typename = std::enable_if_t<!std::is_trivially_copyable<T>::value>>
                T getComponent(size_t index);

                template<typename T>
                size_t setComponent(size_t index, const T& update);

                /**
                 * @brief Retrieves a reference to an unattached component.
                 * 
                 * @details Used in error cases when attempting to access unavailable components.
                 * 
                 * @tparam T The type of base component to be retrieved.
                 * @return Reference to the unattached component.
                 */
                template<typename T>
                T& getDefaultComponent()
                {
                    return *reinterpret_cast<T*>(&components[sizeof(bool)]);
                }
            };

            /**
             * @brief A manager for `ComponentArray`s of every type.
             * 
             * @details Handles organizing and locating component pools based on their type.
             * Works with `ComponentType` in order to give each pool data type a unique ID.
             */
            struct ComponentManager
            {
                static inline uint32_t cidCount = 0; /** @brief Holds the number of registered component types.*/

                static inline std::vector<size_t> spaceBuffer = {};        /** @brief A temporary vector that holds the size of each component per pool.*/
                static inline std::vector<bool> complexBuffer = {};        /** @brief A temporary vector that stores whether a component type is copyable.*/

                std::vector<ComponentArray> componentArrays; /** @brief A vector of component pools.*/
                std::vector<std::vector<size_t>> indexMaps;


                static size_t length(const ComponentManager& data)
                {
                    return 
                        object::length(data.cidCount) +
                        object::length(data.spaceBuffer) +
                        object::length(data.complexBuffer) +
                        object::length(data.componentArrays) +
                        object::length(data.indexMaps);
                }

                static size_t serialize(const ComponentManager& value, std::vector<uint8_t>& stream, size_t index)
                {
                    size_t count = 0;

                    count += object::serialize(value.cidCount, stream, index + count);
                    count += object::serialize(value.spaceBuffer, stream, index + count);
                    count += object::serialize(value.complexBuffer, stream, index + count);
                    count += object::serialize(value.componentArrays, stream, index + count);
                    count += object::serialize(value.indexMaps, stream, index + count);

                    return count;
                }

                static ComponentManager deserialize(std::vector<uint8_t>& stream, size_t index)
                {
                    ComponentManager result = ComponentManager();
                    size_t count = 0;

                    // STATIC: special consideration may be required
                    result.cidCount = object::deserialize<uint32_t>(stream, index + count);
                    count += object::length(result.cidCount);

                    // STATIC: special consideration may be required
                    result.spaceBuffer = object::deserialize<std::vector<size_t>>(stream, index + count);
                    count += object::length(result.spaceBuffer);

                    // STATIC: special consideration may be required
                    result.complexBuffer = object::deserialize<std::vector<bool>>(stream, index + count);
                    count += object::length(result.complexBuffer);

                    result.componentArrays = object::deserialize<std::vector<ComponentArray>>(stream, index + count);
                    count += object::length(result.componentArrays);

                    result.indexMaps = object::deserialize<std::vector<std::vector<size_t>>>(stream, index + count);
                    count += object::length(result.indexMaps);

                    return result;
                }


                ComponentManager() {}

                /**
                 * @brief Constructor for struct `ComponentManager`.
                 * 
                 * @details Further initializes each component pool with information 
                 *          related to the type of data they hold.
                 */
                ComponentManager(entity identifiers) : indexMaps(cidCount, std::vector<size_t>(identifiers))
                {
                    componentArrays = std::vector<ComponentArray>();
                    for(uint32_t i=0; i<cidCount; i++)
                    {
                        // space is allocated for an empty object of type T; this object can be used for error-handling
                        componentArrays.push_back(ComponentArray(spaceBuffer[i], complexBuffer[i]));
                    }
                }

                void addID()
                {
                    for(uint32_t i=0; i<cidCount; i++)
                    {
                        indexMaps[i].push_back(-1);
                    }
                }

                /**
                 * @brief Signals each component pool that an `Entity` has been removed.
                 */
                void removeID(entity e)
                {
                    for(uint32_t cid=0; cid<cidCount; cid++)
                    {
                        size_t index = indexMaps[cid][e];
                        if(index == (size_t)-1)
                            continue;

                        remove(cid, index, e);
                    }
                }

                void update(entity identifiers)
                {
                    size_t size = indexMaps.size();
                    while(size < cidCount)
                    {
                        indexMaps.push_back(std::vector<size_t>(identifiers));
                        componentArrays.push_back(ComponentArray(spaceBuffer[size], complexBuffer[size]));
                        size++;
                    }
                }

                bool complex(uint32_t cid)
                {
                    return componentArrays[cid].complex();
                }

                /**
                 * @brief Attaches data based off a provided data type and `Entity`.
                 * 
                 * @details Accesses the proper component pool and allocates space for a new component that
                 *          can be accessed with the value of `entity`. Also verifies that the provided entity
                 *          is `active` (a.k.a. is has not been removed).
                 * 
                 * @tparam T The type of component pool to be accessed and the type of component to be added to entity.
                 * @param e An `Entity` made by the `createEntity` function. Used to index component data in respective pool.
                 * @param component The component data to be linked to `entity`.
                 * @param active Boolean to determine whether the `entity` is usable.
                 * @return Reference to the component in its component pool.
                 */
                template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
                T& addComponent(entity e, uint32_t cid, const T& component)
                {
                    ComponentArray& array = componentArrays[cid];
                    size_t& index = indexMaps[cid][e];
                    T& result = array.addComponent<T>(index, component);
                    return result;
                }


                template<typename T, typename = std::enable_if_t<!std::is_trivially_copyable<T>::value>>
                T addComponent(entity e, uint32_t cid, const T& component)
                {
                    ComponentArray& array = componentArrays[cid];       
                    return array.addComponent<T>(indexMaps[cid][e], component);
                }

                template<typename T>
                void share(entity e, entity share, uint32_t cid)
                {
                    if(indexMaps[cid][e] != -1)
                    {
                        removeComponent<T>(e);
                    }
                    indexMaps[cid][e] = indexMaps[cid][share];
                }

                /**
                 * @brief Retrieves data based off a provided data type and  `Entity`.
                 * 
                 * @details Accesses the proper component pool and retrieves a piece of data of type T
                 *          attached to the `entity` if such data exists.
                 * 
                 * @tparam T The type of component pool to be accessed and the type of component to be retrieved.
                 * @param entity An `Entity` made by the `createEntity` function. Used to index component data in respective pool.
                 * @return Reference to the component in its component pool.
                 */
                template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
                T& getComponent(entity e, uint32_t cid)
                {
                    return componentArrays[cid].getComponent<T>(indexMaps[cid][e]);
                }

                template<typename T, typename = std::enable_if_t<!std::is_trivially_copyable<T>::value>>
                T getComponent(entity e, uint32_t cid)
                {
                    return componentArrays[cid].getComponent<T>(indexMaps[cid][e]);
                }


                template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
                T& getComponentAt(entity e, uint32_t cid, size_t index)
                {
                    size_t idx = indexMaps[cid][e];
                    ComponentArray& array = componentArrays[cid];
                    return object::deserialize<T>(array.components, idx + 2*sizeof(size_t) + sizeof(T) * index);
                }


                template<typename T>
                size_t getCompressedIndex(entity e, uint32_t cid)
                {
                    return indexMaps[cid][e];
                }


                template<typename T>
                T& getDefaultComponent(uint32_t cid)
                {
                    return componentArrays[cid].getDefaultComponent<T>();
                }

                template<typename T>
                bool containsComponent(entity e, uint32_t cid)
                {
                    return (indexMaps[cid][e] != -1);
                }

                /**
                 * @brief Removes data based off a provided data type and `Entity`.
                 * 
                 * @details Accesses the proper component pool, deletes, and deallocates a piece of data 
                 *          of type T attached to the `entity` if such data exists.
                 * 
                 * @tparam T The type of component pool to be accessed and the type of component to be deleted.
                 * @param entity An `Entity` made by the `createEntity` function. Used to index component data in respective pool.
                 * @return Copy of the deleted component.
                 */
                template<typename T>
                T removeComponent(entity e, uint32_t cid)
                {
                    T result = getComponent<T>(e);
                    size_t index = indexMaps[cid][e];

                    remove(cid, index, e);

                    return result;
                }
                
                template<typename T>
                void setComponent(entity e, uint32_t cid, const T& update)
                {
                    size_t index = indexMaps[cid][e];
                    size_t offset = componentArrays[cid].setComponent<T>(index, update);

                    size_t size = indexMaps[cid].size();
                    for(size_t i=0; i<size; i++)
                    {
                        if(indexMaps[cid][i] > index)
                            indexMaps[cid][i] += offset;
                    }
                }
                
                /**
                 * @brief Creates and returns a new 32-bit ID.
                 * 
                 * @details When a new `ComponentType<T>` is referenced, it is statically initialized with
                 *          this function.
                 * 
                 * @tparam T The data type whose size is stored.
                 * @return A unique 32-bit ID.
                 */
                template<typename T>
                static uint32_t newId()
                {
                    // whenever the compiler finds a new ComponentType, this function is called
                    uint32_t index = cidCount;
                    spaceBuffer.push_back(sizeof(T));
                    complexBuffer.push_back(std::is_trivially_copyable<T>());
                    cidCount++;
                    return index;
                }

                private:
                    void remove(uint32_t cid, size_t index, entity e)
                    {
                        ComponentArray& array = componentArrays[cid];
                        array.count--;
                        size_t offset = array.overwrite(index);
                        
                        indexMaps[cid][e] = -1;

                        size_t size = indexMaps[cid].size();
                        for(size_t i=0; i<size; i++)
                        {
                            if(indexMaps[cid][i] > index && indexMaps[cid][i] != (size_t)-1)
                            {
                                indexMaps[cid][i] -= offset;
                            }
                        } 
                    }
            };


            /**
             * @brief Assigns a unique ID for data types to be used as components.
             * 
             * @tparam T The type to be given a unique ID.
             */
            template<typename T>
            struct ComponentType
            {
                inline static const uint32_t id = ComponentManager::newId<T>(); /** @brief The unique ID assigned to a type.*/
            };


            struct SystemFunction
            {
                void (*func)(ecs&, system&, void *);
                bool active;

                SystemFunction()
                {
                    func = [](object::ecs&, system&, void *){};
                    active = true;
                }
            };


            struct FunctionID
            {
                uint32_t id;
                uint8_t function;

                FunctionID() {}

                FunctionID(uint32_t id__, uint8_t function__)
                {
                    id = id__;
                    function = function__;
                }
            };


            struct SystemToggle
            {
                std::vector<FunctionID> functions;

                static size_t length(const SystemToggle& data)
                {
                    return
                        object::length(data.functions);
                }

                static size_t serialize(const SystemToggle& value, std::vector<uint8_t>& stream, size_t index)
                {
                    size_t count = 0;

                    count += object::serialize(value.functions, stream, index + count);       // STATIC

                    return count;
                }

                static SystemToggle deserialize(std::vector<uint8_t>& stream, size_t index)
                {
                    SystemToggle result = SystemToggle();
                    size_t count = 0;

                    result.functions = object::deserialize<std::vector<FunctionID>>(stream, index + count);
                    count += object::length(result.functions);

                    return result;
                }

                SystemToggle()
                {
                    functions = std::vector<FunctionID>();
                }
            };


            struct SystemSupplement
            {
                std::vector<uint32_t> requirement;
                std::vector<size_t> indexMap;
                std::vector<entity> reverseIndexMap;
                void (*insertion)(ecs&, entity, std::vector<entity>&, std::vector<size_t>&);

                static size_t length(const SystemSupplement& data)
                {
                    return
                        object::length(data.requirement) +
                        object::length(data.indexMap) +
                        object::length(data.reverseIndexMap);
                }

                static size_t serialize(const SystemSupplement& value, std::vector<uint8_t>& stream, size_t index)
                {
                    size_t count = 0;

                    count += object::serialize(value.requirement, stream, index + count);
                    count += object::serialize(value.indexMap, stream, index + count);
                    count += object::serialize(value.reverseIndexMap, stream, index + count);

                    return count;
                }

                static SystemSupplement deserialize(std::vector<uint8_t>& stream, size_t index)
                {
                    SystemSupplement result = SystemSupplement();
                    size_t count = 0;

                    result.requirement = object::deserialize<std::vector<uint32_t>>(stream, index + count);
                    count += object::length(result.requirement);

                    result.indexMap = object::deserialize<std::vector<size_t>>(stream, index + count);
                    count += object::length(result.indexMap);

                    result.reverseIndexMap = object::deserialize<std::vector<uint32_t>>(stream, index + count);
                    count += object::length(result.reverseIndexMap);

                    return result;
                }


                SystemSupplement() {}

                SystemSupplement(entity numberOfEntities)
                {
                    requirement = std::vector<uint32_t>();
                    indexMap = std::vector<size_t>(numberOfEntities);
                    reverseIndexMap = std::vector<entity>();
                    insertion = [] (ecs&, entity e, std::vector<entity>& entities, std::vector<size_t>& map)
                    {
                        map[e] = entities.size();
                        entities.push_back(e);
                    };
                }

                void insert(ecs& container, entity e)
                {
                    insertion(container, e, reverseIndexMap, indexMap);
                }
            
                void extract(entity e)
                {
                    size_t revSize = reverseIndexMap.size();

                    entity last = reverseIndexMap[revSize-1];
                    reverseIndexMap[indexMap[e]] = last;
                    indexMap[last] = indexMap[e];

                    reverseIndexMap.pop_back();
                    indexMap[e] = -1;
                }
            
                void clearEntities()
                {
                    indexMap = std::vector<size_t>();
                    reverseIndexMap = std::vector<entity>();
                }
            
                bool component()
                {
                    return requirement.size();
                }
            };


            struct SystemManager
            {
                static inline uint32_t idCount = 0;
                static inline std::vector<size_t> spaceBuffer = {};

                uint8_t functionIndex = 0;

                std::vector<system> stores;
                std::vector<SystemSupplement> supplements;
                std::vector<uint32_t> indexMap;
                std::vector<SystemToggle> toggles;


                static size_t length(const SystemManager& data)
                {
                    return
                        object::length(data.idCount) +
                        object::length(data.functionIndex) +
                        object::length(data.spaceBuffer) +
                        object::length(data.stores) +
                        object::length(data.supplements) +
                        object::length(data.indexMap) +
                        object::length(data.toggles);
                }

                static size_t serialize(const SystemManager& value, std::vector<uint8_t>& stream, size_t index)
                {
                    size_t count = 0;

                    count += object::serialize(value.idCount, stream, index + count);       // STATIC
                    count += object::serialize(value.functionIndex, stream, index + count); // STATIC
                    count += object::serialize(value.spaceBuffer, stream, index + count);   // STATIC
                    count += object::serialize(value.stores, stream, index + count);
                    count += object::serialize(value.supplements, stream, index + count);
                    count += object::serialize(value.indexMap, stream, index + count);
                    count += object::serialize(value.toggles, stream, index + count);

                    return count;
                }

                static SystemManager deserialize(std::vector<uint8_t>& stream, size_t index)
                {
                    SystemManager result = SystemManager();
                    size_t count = 0;

                    // STATIC: special consideration may be required
                    result.idCount = object::deserialize<uint32_t>(stream, index + count);
                    count += object::length(result.idCount);

                    // STATIC: special consideration may be required
                    result.functionIndex = object::deserialize<uint8_t>(stream, index + count);
                    count += object::length(result.functionIndex);
                    
                    // STATIC: special consideration may be required
                    result.spaceBuffer = object::deserialize<std::vector<size_t>>(stream, index + count);
                    count += object::length(result.spaceBuffer);

                    result.stores = object::deserialize<std::vector<system>>(stream, index + count);
                    count += object::length(result.stores);

                    result.supplements = object::deserialize<std::vector<SystemSupplement>>(stream, index + count);
                    count += object::length(result.supplements);

                    result.indexMap = object::deserialize<std::vector<uint32_t>>(stream, index + count);
                    count += object::length(result.indexMap);

                    result.toggles = object::deserialize<std::vector<SystemToggle>>(stream, index + count);
                    count += object::length(result.toggles);

                    return result;
                }


                SystemManager() {}

                SystemManager(entity numberOfEntities) : stores(idCount, system(functionIndex)), supplements(idCount, SystemSupplement(numberOfEntities)), indexMap(idCount, -1) {}

                void runFunction(ecs& container, uint8_t index, void *data)
                {
                    for(size_t i=0; i<stores.size(); i++)
                    {                  
                        if(indexMap[i] != (entity)-1 && stores[indexMap[i]].isInitialized())
                        {
                            stores[indexMap[i]].runFunction(container, index, data);
                        }
                    }
                }

                bool bitmapMatches(entity index, const std::vector<bool>& bitmap) const
                {
                    const std::vector<uint32_t>& requirement = supplements[index].requirement;
                    if(requirement.empty())
                    {
                        return false;
                    }
                    
                    for(uint32_t req : requirement)
                    {
                        if(!bitmap[req])
                            return false;
                    }
                    return true;
                }

                bool bitmapMatches(entity index, const std::vector<bool>& bitmap, uint32_t bit) const
                {
                    const std::vector<uint32_t>& requirement = supplements[index].requirement;
                    if(requirement.empty())
                    {
                        return false;
                    }
                    
                    bool valid = false;
                    for(uint32_t req : requirement)
                    {
                        if(!bitmap[req])
                        {
                            return false;
                        }
                        if(req == bit)
                            valid = true;
                    }
                    return valid;
                }

                void setInsertion(uint32_t index, void (*insert)(ecs&, entity, std::vector<entity>&, std::vector<size_t>&))
                {
                    supplements[index].insertion = insert;
                }

                void setActive(uint32_t index, uint8_t func, bool state)
                {
                    stores[index].setActive(func, state);
                }

                void setActive(uint32_t index, bool state)
                {
                    stores[index].setActive(state);
                }

                void toggleFunction(uint32_t index, uint8_t func)
                {
                    setActive(index, func, !functionActive(index, func));
                }

                void toggleFunctions(uint32_t index)
                {
                    stores[index].toggle();
                }

                bool functionActive(uint32_t index, uint8_t func)
                {
                    return stores[index].functionActive(func);
                }

                uint32_t createToggle()
                {
                    uint32_t result = toggles.size();
                    toggles.push_back(SystemToggle());
                    return result;
                }

                void addToggle(uint32_t toggle, uint32_t sys, uint8_t func)
                {
                    toggles[toggle].functions.push_back(FunctionID(sys, func));
                }
                
                void addToggles(uint32_t toggle, uint32_t sys)
                {
                    for(int i=0; i<functionIndex; i++)
                        toggles[toggle].functions.push_back(FunctionID(sys, i));
                }

                void toggle(uint32_t index)
                {
                    std::vector<FunctionID> functions = toggles[index].functions;
                    for(const auto& func : functions)
                    {
                        toggleFunction(func.id, func.function);
                    }
                }

                /**
                 * @brief Signals each system that a new `Entity` has been created.
                 */
                void addEntity()
                {
                    for(uint32_t i=0; i<supplements.size(); i++)
                    {
                        supplements[i].indexMap.push_back(-1);
                    }
                }

                void insertEntity(ecs& container, entity e, uint32_t index)
                {
                    supplements[index].insert(container, e);
                }

                void extractEntity(entity e, const std::vector<bool>& bitmap)
                {
                    for(uint32_t i=0; i<supplements.size(); i++)
                    {
                        if(bitmapMatches(i, bitmap))
                        {
                            supplements[i].extract(e);
                        }
                    }
                }

                void componentRemoved(entity e, uint32_t bit, const std::vector<bool>& bitmap)
                {
                    for(uint32_t i=0; i<supplements.size(); i++)
                    {
                        if(bitmapMatches(i, bitmap, bit))
                        {
                            size_t revSize = supplements[i].reverseIndexMap.size();
                            entity last = supplements[i].reverseIndexMap[revSize-1];
                            supplements[i].reverseIndexMap[supplements[i].indexMap[e]] = last;
                            supplements[i].indexMap[last] = supplements[i].indexMap[e];

                            supplements[i].reverseIndexMap.pop_back();
                            supplements[i].indexMap[e] = -1;
                        }
                    }
                }


                bool contains(uint32_t index)
                {
                    return index < stores.size();
                }

                bool containsFunction(uint32_t index)
                {
                    return index < functionIndex;
                }

                bool initialized(uint32_t index)
                {
                    return stores[index].isInitialized();
                }

                uint8_t createSystemFunction()
                {
                    for(system& store : stores)
                    {
                        store.createFunction();
                    }
                    return functionIndex++;
                }

                void update(entity numberOfEntities)
                {
                    while(supplements.size() < idCount)
                    {
                        stores.push_back(system(functionIndex));
                        supplements.push_back(SystemSupplement(numberOfEntities));
                        indexMap.push_back(-1);
                    }
                }

                template<typename T>
                system& createSystem(const T& instance, int32_t priority, uint32_t id)
                {
                    uint32_t max = idCount-1;
                    uint32_t index = max/2;
                    uint32_t total = index, buffer;
                    while (index != 0)
                    {
                        buffer = index;
                        index = index/2;

                        int32_t priorityTwo = (indexMap[total] == (entity)-1) ? INT_MAX : stores[indexMap[total]].priority;
                        if(priorityTwo > priority)
                        {
                            total -= (index+(buffer % 2));
                        }
                        else
                        {
                            total += (index+(buffer % 2));
                        }
                    }

                    
                    if(id >= total)
                    {
                        for(uint32_t i=max; i>total; i--)
                        {
                            indexMap[i] = indexMap[i-1];
                        }
                    }
                    else
                    {
                        for(uint32_t i=0; i<total; i++)
                        {
                            indexMap[i] = indexMap[i+1];
                        }
                    }

                    indexMap[total] = id;

                    stores[id].initialize<T>(instance);
                    stores[id].priority = priority;

                    return stores[id];
                }

                system& getSystem(uint32_t index)
                {
                    return stores[index];
                }

                template<typename T>
                std::vector<size_t>& getIndexMap()
                {
                    uint32_t id = SystemType<T>::id;
                    return supplements[id].indexMap;
                }

                template<typename T>
                std::vector<entity>& entities()
                {
                    uint32_t id = SystemType<T>::id;
                    return supplements[id].reverseIndexMap;
                }

                void clearEntities()
                {
                    for(size_t i=0; i<supplements.size(); i++)
                    {
                        supplements[i].clearEntities();
                    }
                }

                template<typename T, typename... Args>
                void addRequirements();

                const std::vector<uint32_t>& getRequirement(uint32_t index) const
                {
                    return supplements[index].requirement;
                }

                template<typename T>
                static uint32_t newId()
                {
                    uint32_t index = idCount;
                    // whenever the compiler finds a new ComponentType, this function is called
                    spaceBuffer.push_back(sizeof(T));
                    idCount++;
                    return index;
                }

                private:
                    template<typename Sys, typename S>
                    void addRequirement(uint32_t id)
                    {
                        uint32_t componentId = ComponentType<S>::id;
                        std::vector<uint32_t>& requirement = supplements[id].requirement;

                        for(uint32_t index = 0; index < requirement.size(); index++)
                        {
                            if(requirement[index] == componentId)
                                return;
                        }
                        requirement.push_back(componentId);
                    }

                    // as the function runs recursively, `First` is set as each type from `Args` one by one until `Args` is empty
                    template<typename Sys, typename First, typename... Args>
                    void addRequirementsRecursive(uint32_t id)
                    {
                        addRequirement<Sys, First>(id);
                        addRequirementsRecursive<Sys, Args...>(id);
                    }

                    // this is the end point for `addRequirementsRecursive` (it only runs when there are no more arguments to proceess)
                    template<typename Sys, typename... Args>
                    std::enable_if_t<sizeof...(Args) == 0> addRequirementsRecursive(uint32_t) {}
            };


            /**
             * @brief Assigns a unique ID for data types to be used as systems.
             * 
             * @tparam T The type to be given a unique ID.
             */
            template<typename T>
            struct SystemType
            {
                inline static uint32_t id = SystemManager::newId<T>(); /** @brief The unique ID assigned to a type.*/
            };


            EntityManager entityManager;
            ComponentManager componentManager;
            SystemManager systemManager;

            static inline uint16_t error = 0;

            void addComponentConfiguration(entity e, uint32_t id)
            {
                entityManager.setComponentBit(e, id, true);
                std::vector<bool>& bitmap = entityManager.getBitmap(e);

                for(uint32_t i=0; i<systemManager.stores.size(); i++)
                {
                    if(systemManager.bitmapMatches(i, bitmap, id))
                    {
                        systemManager.insertEntity(*this, e, i);
                    }
                }
            }
    };      

    template<typename T, typename>
    T& ecs::ComponentArray::addComponent(size_t& index, const T& component)
    {
        #ifndef ECS_DEBUG_OFF
            if(index != (size_t)-1)
            {
                ecs::error = 1;
                return object::deserialize<T>(components, index);
            }
        #endif

        size_t arraySize = components.size();

        // copy data from `&component` to the newly allocated space

        components.resize(arraySize + sizeof(T));
        object::serialize(component, components, arraySize);

        // save the index for the entity
        index = arraySize;

        // could use `getComponent<T>()`, this avoids unnecessary index check
        return object::deserialize<T>(components, arraySize);
    }

    template<typename T, typename>
    T ecs::ComponentArray::addComponent(size_t& index, const T& component)
    {
        #ifndef ECS_DEBUG_OFF
            if(index != (size_t)-1)
            {
                ecs::error = 1;
                return T();
            }
        #endif

        size_t arraySize = components.size();

        // copy data from `&component` to the newly allocated space
        size_t length = Serialization<T>::length(component);

        components.resize(arraySize + sizeof(size_t) + length);
        object::serialize(component, components, arraySize, length);

        // save the index for the entity
        index = arraySize;

        // could use `getComponent<T>()`, this avoids unnecessary index check
        return object::deserialize<T>(components, index);
    }


    template<typename T, typename>
    T& ecs::ComponentArray::getComponent(size_t index)
    {
        // indices set to `-1` represent uninitialized components; this triggers an error
        #ifndef ECS_DEBUG_OFF
            if(index == (size_t)-1)
            {
                ecs::error = 2;
                
                // an empty component is stored at the start of each array; this is returned when an error occurs
                return getDefaultComponent<T>();
            }
        #endif
        return object::deserialize<T>(components, index);
    }

    template<typename T, typename>
    T ecs::ComponentArray::getComponent(size_t index)
    {
        #ifndef ECS_DEBUG_OFF
            if(index == (size_t)-1)
            {
                ecs::error = 2;
                T();
            }
        #endif
        return object::deserialize<T>(components, index);
    }


    template<typename T>
    size_t ecs::ComponentArray::setComponent(size_t index, const T& update)
    {
        // indices set to `-1` represent uninitialized components; this triggers an error
        #ifndef ECS_DEBUG_OFF
            if(index == (size_t)-1)
            {
                ecs::error = 2;
                return 0;
            }
        #endif
        
        size_t length = object::length(update);
        size_t offset = object::resize<T>(length, components, index);
        object::serialize<T>(update, components, index, length);
        
        return offset;
    }

    template<typename T, typename... Args>
    void ecs::SystemManager::addRequirements()
    {
        uint32_t id = SystemType<T>::id;
        addRequirementsRecursive<T, Args...>(id);
    }
}