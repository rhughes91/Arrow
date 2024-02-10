#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <cstring>
#include <string>
#include <typeinfo>
#include <stdint.h>
#include <vector>

// #define ECS_DEBUG_OFF

using entity = uint32_t; /** @brief Alias for a 32-bit unsigned value.*/

template<typename T>
struct Serialization
{
    static size_t (*length)(const T&);                                   /** @brief A user-defined function to define the length of provided data T.*/
    static size_t (*serialize)(const T&, std::vector<uint8_t>&, size_t); /** @brief A user-defined function to define the serialization of provided data T.*/
    static T (*deserialize)(std::vector<uint8_t>&, size_t);              /** @brief A user-defined functoin to define the deserialization of provided data T.*/
};

void printSerializationError(const std::string& ); /** @brief Prints a serialization-related error to the console.*/

/**
 * @brief Defines a default value for each Serialization type. It always prints and error.
 * 
 * @tparam T The Serialization type.
 * @return Default length for all types (0).
 */
template<typename T>
size_t defaultLengthSetter(const T&)
{
    printSerializationError("ERROR: " + std::string(typeid(T).name()) + std::string(" does not have a function to define serialization length.\n"));
    return 0;
}

template<typename T>
size_t (*Serialization<T>::length)(const T&) = defaultLengthSetter;

/**
 * @brief Defines a default value for each Serialization type. It always prints and error.
 * 
 * @tparam T The Serialization type.
 * @return Default length for all types (0).
 */
template<typename T>
size_t defaultSerialization(const T&, std::vector<uint8_t>&, size_t)
{
    printSerializationError("ERROR: " + std::string(typeid(T).name()) + " does not have a defined serialization function.\n");
    return 0;
}

template<typename T>
size_t(*Serialization<T>::serialize)(const T&, std::vector<uint8_t>&, size_t) = defaultSerialization;

/**
 * @brief Defines a default value for each Serialization type. It always prints and error.
 * 
 * @tparam T The Serialization type.
 * @return Returns the default constructor for type T.
 */
template<typename T>
T defaultDeserialization(std::vector<uint8_t>&, size_t)
{
    printSerializationError("ERROR: " + std::string(typeid(T).name()) + " does not have a defined deserialization function.\n");
    return T();
}

template<typename T>
T (*Serialization<T>::deserialize)(std::vector<uint8_t>&, size_t) = defaultDeserialization;


namespace object
{
    struct ecs;

    /**
     * @brief Assigns a unique ID for data types to be used as components.
     * 
     * @tparam T The type to be given a unique ID.
     */
    template<typename T>
    struct ComponentType
    {
        static const uint32_t id; /** @brief The unique ID assigned to a type.*/
    };

    /**
     * @brief Assigns a unique ID for data types to be used as systems.
     * 
     * @tparam T The type to be given a unique ID.
     */
    template<typename T>
    struct SystemType
    {
        static uint32_t id; /** @brief The unique ID assigned to a type.*/
    };

    /**
     * @brief Returns the length of the given value in regard to serialization. A custom length function is called if type T is not trivially copyable.
     * 
     * @tparam T The type of the given value.
     * @param value The value whose length is determined for serialization/deserialization.
     * @return Returns the length of `value`.
     */
    template <typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, int> = 0>
    size_t length(const T& value)
    {
        return sizeof(T);
    }

    template <typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, int> = 0>
    size_t length(const T& value)
    {
        return Serialization<T>::length(value) + sizeof(size_t);
    }

    /**
     * @brief Serializes the given value into the given character stream. A custom serialization function is called if type T is not trivially copyable.
     * 
     * @tparam T The type of the given value.
     * @param value The value which is serializaed.
     * @param stream The character stream into which `value` is serialized.
     * @param index The location where `value` is serialized into `stream`.
     * @param length The length of the given value.
     * @return Returns the length of `value`.
     */
    template <typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, int> = 0>
    size_t serialize(const T& value, std::vector<uint8_t>& stream, size_t index, size_t length = sizeof(T))
    {
        std::memcpy(&stream[index], &value, length);
        return length;
    }

    template <typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, int> = 0>
    size_t serialize(const T& value, std::vector<uint8_t>& stream, size_t index, size_t length = sizeof(T))
    {
        object::serialize(length, stream, index);
        return Serialization<T>::serialize(value, stream, index + sizeof(size_t)) + sizeof(size_t);
    }

    /**
     * @brief Deserializaes the given value from the given character stream. A custom deserialization function is called if type T is not trivially copyable.
     * 
     * @tparam T The type of the given value.
     * @param stream The character stream from which the value is deserialized.
     * @param index The location where the value will be deserialized from `stream`.
     * @return Returns the deserialized value (only returns a reference if values of type T are trivially copyable).
     */
    template <typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, int> = 0>
    T& deserialize(std::vector<uint8_t>& stream, size_t index)
    {
        return *reinterpret_cast<T*>(&stream[index]);
    }

    template <typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, int> = 0>
    T deserialize(std::vector<uint8_t>& stream, size_t index)
    {
        return Serialization<T>::deserialize(stream, index + sizeof(size_t));
    }

    /**
     * @brief Resizes the given character stream based on the length of a value to be inserted and replaced. The stream is only resized if type T is not trivially copyable.
     * 
     * @tparam T The type which determines which `resize` function to use.
     * @param index The index of the value to be replaced (only used in non-trivially copyable cases).
     * @return Returns the difference in length between the new and old value.
     */
    template <typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, int> = 0>
    size_t resize(size_t, std::vector<uint8_t>&, size_t)
    {
        return 0;
    }

    template <typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, int> = 0>
    size_t resize(size_t length, std::vector<uint8_t>& stream, size_t index)
    {
        size_t original = object::deserialize<size_t>(stream, index);
        size_t offset = length - original;

        std::vector<uint8_t> copy = stream;
        
        if(original < length)
        {
            stream.resize(stream.size() + offset);
            std::memcpy(&stream[index + length], &copy[index + original], copy.size() - index - original);
        }
        else if(original > length)
        {
            std::memcpy(&stream[index + length], &copy[index + original], copy.size() - index - original);
            stream.resize(stream.size() + offset);
        }
        
        return offset;
    }


    /**
     * @brief An entity-component-system; handles all the operations needed to create entities, attach components, and run systems.
     */
    struct ecs
    {
        /**
         * @brief Describes archetypes for entities to be divided into, and it runs function on those collections.
         * 
         * @details Allows the user to iterate over `Entity`s with common components in user-defined functions, 
         * holds a static instance of the `system`'s parent data type.
         */
        struct system
        {
            /**
             * @brief Constructor for struct 'system'.
             */
            system() {}

            /**
             * @brief Constructor for struct 'system'.
             * 
             * @param functionSize Describes how much space should allocated for user-defined functions.
             */
            system(uint8_t functionSize)
            {
                m_functions = std::vector<void (*)(object::ecs&, system&)>(functionSize, [](object::ecs&, system&){});
                m_initialized = false;
            }

            /**
             * @brief Finishes initializing this `system`.
             * 
             * @details The `system`'s static instance, its constructor, and its destructor are fully 
             *          initialized and set.
             * 
             * @tparam T The type of `system`'s static instance.
             * @param instance The initial data of `system's` static instance.
             */
            template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
            void initialize(const T& instance)
            {
                m_initialized = true;
                m_instance.resize(sizeof(T));
                object::serialize<T>(instance, m_instance, 0);
            }

            /**
             * @brief Finishes initializing this `system` (non-copyable type exclusive).
             * 
             * @details The `system`'s static instance, its constructor, and its destructor are fully 
             *          initialized and set.
             * 
             * @tparam T The type of `system`'s static instance.
             * @param instance The initial data of `system's` static instance.
             * @param temp Differentiates function definition from other `initialize` function.
             */
            template<typename T, typename = std::enable_if_t<!std::is_trivially_copyable<T>::value>>
            void initialize(const T instance, bool temp = false)
            {
                m_initialized = true;
                m_instance.resize(object::length(instance) + sizeof(size_t));
                object::serialize<T>(instance, m_instance, 0);
            }

            /**
             * @brief Determines whether the current `system` has been fully initialized.
             * 
             * @return The initialization state of this `system`.
             */
            bool initialized() const
            {
                return m_initialized;
            }

            /**
             * @brief Creates a new usable space for a user-defined function.
             */
            void createFunction()
            {
                m_functions.push_back([](ecs&, system&){});
            }


            /**
             * @brief Sets a user-defined function pointer at a certain index.
             * 
             * @param index The index where the function pointer will be placed.
             * @param function The new function pointer that will be stored.
             */
            void setFunction(uint8_t index, void (*function)(ecs&, system&))
            {
                m_functions[index] = function;
            }


            /**
             * @brief Runs the user-defined or default function at a certain index.
             * 
             * @param index The index of the function to be run.
             */
            void runFunction(ecs& container, uint8_t index)
            {
                m_functions[index](container, *this);
            }

            /**
             * @brief Converts this `system`'s static instance from unsigned 8-bit data into its original type.
             * 
             * @tparam T The original data type of this `system`'s static instance.
             * @return The 8-bit data transformed into a usable type.
             */
            template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
            T& getInstance()
            {
                return object::deserialize<T>(m_instance, 0);
            }

            /**
             * @brief Converts this `system`'s static instance from unsigned 8-bit data into its original type (non-copyable type exclusive).
             * 
             * @tparam T The original data type of this `system`'s static instance.
             * @return The 8-bit data transformed into a usable type.
             */
            template<typename T, typename = std::enable_if_t<!std::is_trivially_copyable<T>::value>>
            T getInstance()
            {
                return object::deserialize<T>(m_instance, 0);
            }

            /**
             * @brief Exchanges this `system`'s static instance for a new value.
             * 
             * @tparam T The original data type of this `system`'s static instance.
             * @param instance Sets the current 'system''s instance to the this value.
             */
            template<typename T>
            void pushInstance(const T& instance)
            {
                m_instance.resize(object::length(instance));
                object::serialize<T>(instance, m_instance, 0);
            }
            
            private:
                bool m_initialized;                               /** @brief Tracks whether the `system` has been fully initialized.*/
                std::vector<uint8_t> m_instance;                  /** @brief The raw static instance data.*/
                std::vector<void (*)(ecs&, system&)> m_functions; /** @brief Various user-defined or default functions that can be run on the `system`.*/
        };

        static uint16_t error; /** @brief Holds an unsigned integral value to describe an error state.*/

        /**
         * @brief Constructor for an `ecs` object.
         * 
         * @details Initializes each of its relevant managers (EntityManager, ComponentManager, SystemManager).
         */
        ecs()
        {
            entity totalEntities = m_entityManager.totalEntityCount();
            m_componentManager = ComponentManager(totalEntities);
            m_systemManager = SystemManager(totalEntities);
        }

        /**
         * @brief Creates and returns a new `entity` id.
         * 
         * @details Initializes a new `entity` instance, and signals each respective manager that an `entity`
         * has been created.
         * 
         * @return Returns the created `entity`.
         */
        entity createEntity()
        {
            m_systemManager.update(m_entityManager.totalEntityCount());
            m_systemManager.addEntity();
            
            m_componentManager.update(m_entityManager.totalEntityCount());
            m_componentManager.addEntity();
            entity e = m_entityManager.createEntity();

            addComponent<bool>(e, true);

            return e;
        }

        /**
         * @brief Removes an `entity` from the `ecs`.
         * 
         * @details Moves the `entity` id to be recycled for future use, deattaches each of the `entity`'s components,
         * and removes the `entity` from each of its `system`s.
         * 
         * @param e The `entity` to be removed.
         */
        void removeEntity(entity e)
        {
            #ifndef ECS_DEBUG_OFF
                if(!m_entityManager.contains(e))
                {
                    error = 6;
                    return;
                }
            #endif

            m_componentManager.removeEntity(e);
            m_systemManager.extractEntity(e, m_entityManager.getBitmap(e));
            m_entityManager.removeEntity(e);
        }

        /**
         * @brief Returns the total number of created `entity`s. This includes `entity`s that have been removed.
         * 
         * @return Returns the number of created `entity`s.
         */
        entity numberOfEntities()
        {
            return m_entityManager.totalEntityCount();
        }

        /**
         * @brief Returns whether an `entity` is considered active.
         * 
         * @param e The `entity` to be tested.
         * @return Returns true if the given `entity` is active.
         */
        bool active(entity e)
        {
            return getComponent<bool>(e);
        }

        /**
         * @brief Changes the given `entity`'s active state to `newState`.
         * 
         * @details An `entity`'s active state determines whether it is attached to a `system` or not; its components
         * are still easily accessible, but they won't automatically be iterated over.
         * 
         * @param e The `entity` whose state will be altered.
         * @param newState The new value of the `entity`'s active state.
         */
        void setActive(entity e, bool newState)
        {
            #ifndef ECS_DEBUG_OFF
                if(!m_entityManager.contains(e))
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
                std::vector<bool>& bitmap = m_entityManager.getBitmap(e);

                for(uint32_t i=0; i<m_systemManager.idCount; i++)
                {
                    if(m_systemManager.bitmapMatches(i, bitmap))
                    {
                        m_systemManager.insertEntity(e, i);
                    }
                }
            }
            else
            {
                m_systemManager.extractEntity(e, m_entityManager.getBitmap(e));
            }
        }

        /**
         * @brief Returns the number of initialized component types.
         * 
         * @return Returns the number of created component ids.
         */
        size_t numberOfComponents()
        {
            return ComponentManager::idCount;
        }

       /**
        * @brief Returns whether the component `T` attached to the given `entity` is active.
        * 
        * @tparam T The type of component which will be tested.
        * @param e The `entity` whose component will be accessed.
        * @return Returns true if the component is active.
        */
        template <typename T>
        bool active(entity e)
        {
            #ifndef ECS_DEBUG_OFF
                if(!m_entityManager.contains(e))
                {
                    error = 6;
                    return false;
                }
            #endif

            return m_entityManager.getBitmap(e)[ComponentType<T>::id];
        }

         /**
         * @brief Changes the active state of the component `T` attached to the given `entity`.
         * 
         * @tparam T The type of component whose state will be altered.
         * @param e The `entity` whose component will be accessed.
         * @param newState The new value of the component's active state.
         */
        template <typename T>
        void setActive(entity e, bool newState)
        {
            #ifndef ECS_DEBUG_OFF
                if(!m_entityManager.contains(e))
                {
                    error = 6;
                    return;
                }
            #endif

            uint32_t id = ComponentType<T>::id;
            std::vector<bool>& bitmap = m_entityManager.getBitmap(e);
            if(bitmap[id] == newState || !m_componentManager.containsComponent<T>(e))
            {
                return;
            }

            if(!bitmap[id])
                bitmap[id] = true;

            if(newState)
            {
                for(int i=0; i<m_systemManager.idCount; i++)
                {
                    if(m_systemManager.bitmapMatches(i, bitmap, id))
                    {
                        m_systemManager.insertEntity(e, i);
                    }
                }
            }
            else
            {
                m_systemManager.componentRemoved(e, id, bitmap);
            }

            if(bitmap[id])
                bitmap[id] = false;
        }

        /**
         * @brief Adds a component of type `T` to the given `entity`.
         * 
         * @tparam T The type of component to be added.
         * @param e The `entity` that the component will be attached to.
         * @param component An initial value for the added component.
         * @return Returns the newly created component (returns a reference if `T` is trivially copyable).
         */
        template <typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, int> = 0>
        T& addComponent(entity e, const T& component = T())
        {
            #ifndef ECS_DEBUG_OFF
                if(!m_entityManager.contains(e))
                {
                    error = 6;
                    return m_componentManager.getComponent<T>(-1);
                }
            #endif

            uint32_t id = ComponentType<T>::id;
            T& result = m_componentManager.addComponent<T>(e, component);
            addComponentConfiguration(e, id);    
            return result;
        }

        template <typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, int> = 0>
        T addComponent(entity e, const T& component = T())
        {
            #ifndef ECS_DEBUG_OFF
                if(!m_entityManager.contains(e))
                {
                    error = 6;
                    return m_componentManager.getComponent<T>(-1);
                }
            #endif

            uint32_t id = ComponentType<T>::id;
            T result = m_componentManager.addComponent<T>(e, component);
            addComponentConfiguration(e, id);
            
            return result;
        }

        /**
         * @brief Shares access of component `T` from one `entity` with another.
         * 
         * @tparam T The type of component to be shared.
         * @param e The `entity` who will be given access to the component.
         * @param share The `entity` who holds the original component.
         */
        template <typename T>
        void shareComponent(entity e, entity share)
        {
            #ifndef ECS_DEBUG_OFF
                if(!m_entityManager.contains(e))
                {
                    error = 6;
                    return;
                }
            #endif

            uint32_t id = ComponentType<T>::id;
            m_componentManager.share<T>(e, share);
            addComponentConfiguration(e, id);
        }

        /**
         * @brief Sets the value of component `T` attached to the given `entity`.
         * 
         * @tparam T The type of component to be updated.
         * @param e The `entity` whose component will be updated.
         * @param update The value component `T` will be set to.
         */
        template<typename T>
        void setComponent(entity e, const T& update)
        {
            #ifndef ECS_DEBUG_OFF
                if(!m_entityManager.contains(e))
                {
                    e = -1;
                    error = 6;
                }
            #endif

            m_componentManager.setComponent<T>(e, update);
        }

        /**
         * @brief Returns component `T` attached to the given `entity`.
         * 
         * @tparam T The type of component to be returned.
         * @param e The `entity` whose component will be accessed.
         * @return Returns the requested component (returns a reference if `T` is trivially copyable).
         */
        template <typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, int> = 0>
        T& getComponent(entity e)
        {
            #ifndef ECS_DEBUG_OFF
                if(!m_entityManager.contains(e))
                {
                    e = -1;
                    error = 6;
                }
            #endif

            return m_componentManager.getComponent<T>(e);
        }

        template <typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, int> = 0>
        T getComponent(entity e)
        {
            #ifndef ECS_DEBUG_OFF
                if(!m_entityManager.contains(e))
                {
                    e = -1;
                    error = 6;
                }
            #endif

            return m_componentManager.getComponent<T>(e);
        }

        /**
         * @brief Returns whether the given `entity` has a component of the given type.
         * 
         * @tparam T The type of component that the `entity` is checked for.
         * @param e The `entity` which is being checked.
         * @return Returns true if the `entity` has the queried component type.
         */
        template<typename T>
        bool containsComponent(entity e)
        {
            return m_componentManager.containsComponent<T>(e);
        }

        /**
         * @brief Removes component T attached to the given `entity`.
         *          * 
         * @tparam T The type of component to be deleted.
         * @param entity The `entity` whose component will be removed.
         * @return Returns a copy of the deleted component.
         */
        template <typename T>
        T removeComponent(entity e)
        {
            #ifndef ECS_DEBUG_OFF
                if(!m_entityManager.contains(e))
                {
                    error = 6;
                    return T();
                }
            #endif

            uint32_t id = ComponentType<T>::id;
            m_systemManager.componentRemoved(e, id, m_entityManager.getBitmap(e));
            m_entityManager.setComponentBit(e, id, false);
                
            return m_componentManager.removeComponent<T>(e);
        }

        /**
         * @brief Creates a new function that every `system` will be run through.
         * 
         * @return Returns the index to the created function type.
         */
        uint8_t createSystemFunction()
        {
            return m_systemManager.createSystemFunction();
        }

        /**
         * @brief Returns the `entity`s attaches to the `system` associated with type T.
         * 
         * @tparam T The type of `system` whose `entity`s will be accessed.
         * @return Returns a reference to the requested `entity` list.
         */
        template <typename T>
        std::vector<entity>& entities()
        {
            return m_systemManager.entities<T>();
        }

        /**
         * @brief Creates a new `system` based on a given type T.
         * 
         * @tparam T The type of system to be created.
         * @tparam Args The requirements the created `system` will have.
         * @param instance The initial value for the created `system` to hold.
         * @param priority The `system`s priority when a created function is run.
         * @return Returns a reference to the created `system`.
         */
        template<typename T, typename... Args>
        system& createSystem(const T& instance = T(), float priority = 0)
        {
            uint32_t& id = SystemType<T>::id;

            m_systemManager.update(m_entityManager.totalEntityCount());
            system& result = m_systemManager.createSystem<T>(instance, priority, id);

            m_systemManager.addRequirements<T, Args...>();
            
            entity totalEntities = m_entityManager.totalEntityCount();
            for(int i=0; i<totalEntities; i++)
            {
                if(m_entityManager.entityActive(i))
                {
                    std::vector<bool> bitmap = m_entityManager.getBitmap(i);
                    if(m_systemManager.bitmapMatches(id, bitmap))
                    {
                        m_systemManager.insertEntity(i, id);
                    }
                }
            }

            return result;
        }

        /**
         * @brief Sets a `system` to have a user-defined insertion for each `entity`.
         * 
         * @param insert The new insertion function.
         */
        template<typename T>
        void setInsertion(void (*insert)(entity, std::vector<entity>&, std::vector<size_t>&))
        {
            m_systemManager.setInsertion(SystemType<T>::id, insert);
        }

        /**
         * @brief Returns the `entity` index map attached to the `system` associated with type T.
         * 
         * @tparam T The type of `system` whose index map will be accessed.
         * @return Returns a reference to the requested index map.
         */
        template<typename T>
        std::vector<size_t>& getMapping()
        {
            return m_systemManager.getIndexMap<T>();
        }

        /**
         * @brief Runs user-defined functions of a certain type on every `system`.
         * 
         * @param index The index of the functions to be run.
         */
        void run(uint8_t index)
        {
            #ifndef ECS_DEBUG_OFF
                if(!m_systemManager.containsFunction(index))
                {
                    error = 5;
                    return;
                }
            #endif
            m_systemManager.runFunction(*this, index);
        }

        /**
         * @brief Returns the current value of the error id.
         * 
         * @return Returns an error identifier.
         */
        static uint16_t getError()
        {
            uint16_t err = error;
            error = 0;
            return err;
        }

        /**
         * @brief Prints an error based on the current value of the error id.
         */
        static void parseError();

        /**
         * @brief Creates and returns a new 32-bit ID.
         * 
         * @details When a new `ComponentType<T>` is referenced, it is statically initialized with this function.
         * 
         * @tparam T The data type whose size is stored.
         * @return A unique 32-bit ID.
         */
        template<typename T>
        static const uint32_t newComponentId()
        {
            return ComponentManager::newId<T>();
        }

        /**
         * @brief Creates a unique id for any data type that is made into a system.
         * 
         * @details When a new `SystemType<T>` is referenced, it is statically initialized with this function.
         * 
         * @tparam T The type which is assigned an id.
         * @return A unique 32-bit ID.
         */
        template<typename T>
        static const uint32_t newSystemId()
        {
            return SystemManager::newId<T>();
        }

        private:
            /**
             * @brief Signals each system that a component with the provided `id` has been added to the given `entity`.
             * 
             * @param e The `entity` who has had a component added.
             * @param id The id of the added component type.
             */
            void addComponentConfiguration(entity e, uint32_t id)
            {
                m_entityManager.setComponentBit(e, id, true);
                std::vector<bool>& bitmap = m_entityManager.getBitmap(e);

                for(uint32_t i=0; i<m_systemManager.idCount; i++)
                {
                    if(m_systemManager.bitmapMatches(i, bitmap, id))
                    {
                        m_systemManager.insertEntity(e, i);
                    }
                }
            }

            /**
             * @brief A component pool that holds various data of a certain type.
             * 
             * @details Although templates are not explicitly used for the struct, a `ComponentArray` can only hold a single type.
             *          A vector of 8-bit unsigned values is used to hold this information.
             *          Data can easily be transformed into and out of this format.
             */
            struct ComponentArray
            {
                size_t componentSize; /** @brief The size of the component the `components` vector holds.*/

                /**
                 * @brief Constructor for `ComponentArray`.
                 * 
                 * @param size The standard size for the type this array will hold.
                 * @param complexity Describes whether this array will hold a trivially copyable type.
                 */
                ComponentArray(size_t size, bool complexity)
                {
                    m_components.resize(sizeof(bool) + size);
                    object::serialize<bool>(!complexity, m_components, 0);
                    componentSize = size;
                }

                /**
                 * @brief Removes the component at the given index.
                 * 
                 * @param index Describes the location of the component to be removed.
                 * @return Returns the size of the removed component.
                 */
                size_t overwrite(size_t index)
                {
                    size_t offset;
                    if(complex())
                    {
                        offset = object::deserialize<size_t>(m_components, index) + sizeof(size_t);
                    }
                    else
                    {
                        offset = componentSize;
                    }

                    std::memcpy(&m_components[index], &m_components[index + offset], m_components.size() - (index + offset));
                    m_components.resize(m_components.size() - offset);

                    return offset;
                }

                /**
                 * @brief Returns whether the data this array holds is trivially copyable.
                 * 
                 * @return Returns true if the data is not trivially copyable.
                 */
                bool complex()
                {
                    return object::deserialize<bool>(m_components, 0);
                }

                /**
                 * @brief Attaches data based off a provided `Entity`.
                 * 
                 * @details Defined after the definition of struct `ecs`. Allows the user to access specific data
                 * using an `Entity` as a unique index. Only single piece of data can be allocated per type per Entity.
                 * 
                 * @tparam T The type of component to be added to `entity`.
                 * @param index The location where the new component should be placed.
                 * @param component The component data to be linked to `entity`.
                 * @return Returns the added component (returns a reference if trivially copyable).
                 */
                template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
                T& addComponent(size_t& index, const T& component);

                template<typename T, typename = std::enable_if_t<!std::is_trivially_copyable<T>::value>>
                T addComponent(size_t& index, const T& component);

                /**
                 * @brief Returns data based off a provided `Entity`.
                 * 
                 * @details Defined after the definition of struct `ecs`. Retrieves a piece of data of type T attached to
                 * the `entity` if such data exists.
                 * 
                 * @tparam T The type of component to be retrieved.
                 * @param index The location where the component should be retrieved.
                 * @return Returns the queried component (returns a reference if trivially copyable).
                 */
                template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
                T& getComponent(size_t index);

                template<typename T, typename = std::enable_if_t<!std::is_trivially_copyable<T>::value>>
                T getComponent(size_t index);

                /**
                 * @brief Sets the component to a new value `update` at the given index.
                 * 
                 * @tparam T The type of component to be updated.
                 * @param index The location of the component to be updated.
                 * @param update The data to replace the old data.
                 * @return Returns the difference in size between the old and new components.
                 */
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
                    return *reinterpret_cast<T*>(&m_components[sizeof(bool)]);
                }

                private:
                    std::vector<uint8_t> m_components; /** @brief An vector of 8-bit unsigned values that holds all the component data.*/
            };


            /**
             * @brief A manager for `ComponentArray`s of every type.
             * 
             * @details Handles organizing and locating component pools based on their type.
             * Works with `ComponentType` in order to give each pool data type a unique ID.
             */
            struct ComponentManager
            {
                static uint32_t idCount; /** @brief Holds the number of registered component types.*/

                /**
                 * @brief Default constructor for `ComponentManager`.
                 * 
                 */
                ComponentManager() {}

                /**
                 * @brief Constructor for struct `ComponentManager`.
                 * 
                 * @details Further initializes each component pool with information 
                 * related to the type of data they hold.
                 */
                ComponentManager(entity numberOfEntities) : m_indexMaps(idCount, std::vector<size_t>(numberOfEntities))
                {
                    for(int i=0; i<idCount; i++)
                    {
                        // space is allocated for an empty object of type T; this object can be used for error-handling
                        m_componentArrays.push_back(ComponentArray(spaceBuffer[i], complexBuffer[i]));
                    }
                }

                /**
                 * @brief Called whenever an entity is created. Allocates space for a new `entity` index.
                 */
                void addEntity()
                {
                    for(uint32_t i=0; i<idCount; i++)
                    {
                        m_indexMaps[i].push_back(-1);
                    }
                }

                /**
                 * @brief Called whenever an entity is removed Deallocates components attached to the given entity.
                 */
                void removeEntity(entity e)
                {
                    for(uint32_t id=0; id<idCount; id++)
                    {
                        size_t index = m_indexMaps[id][e];
                        if(index == (size_t)-1)
                            continue;

                        remove(id, index, e);
                    }
                }

                /**
                 * @brief Updates the sizing of each component pool if entities were created before this manager was initialized.
                 * 
                 * @param numberOfEntities Determines how much index space should be allocated for each pool.
                 */
                void update(entity numberOfEntities)
                {
                    while(m_indexMaps.size() < idCount)
                    {
                        m_indexMaps.push_back(std::vector<size_t>(numberOfEntities));
                        m_componentArrays.push_back(ComponentArray(spaceBuffer[idCount-1], complexBuffer[idCount-1]));
                    }
                }

                /**
                 * @brief Returns whether the array at the given id holds a trivially copyable type.
                 * 
                 * @return Returns true if the array holds non-trivially copyable data.
                 */
                bool complex(uint32_t id)
                {
                    return m_componentArrays[id].complex();
                }

                /**
                 * @brief Attaches data based off a provided data type and `Entity`.
                 * 
                 * @details Accesses the proper component pool and allocates space for a new component that can be accessed with the
                 * value of `entity`. Also verifies that the provided entity is `active` (a.k.a. it has not been removed).
                 * 
                 * @tparam T The type of component pool to be accessed and the type of component to be added to entity.
                 * @param e An `entity` made by the `createEntity` function. Used to index component data in respective pool.
                 * @param component The component data to be linked to `entity`.
                 * @return Returns the added component (returns a reference if trivially copyable).
                 */
                template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
                T& addComponent(entity e, const T& component)
                {
                    uint32_t id = ComponentType<T>::id;

                    ComponentArray& array = m_componentArrays[id];
                    return array.addComponent<T>(m_indexMaps[id][e], component);
                }

                template<typename T, typename = std::enable_if_t<!std::is_trivially_copyable<T>::value>>
                T addComponent(entity e, const T& component)
                {
                    uint32_t id = ComponentType<T>::id;

                    ComponentArray& array = m_componentArrays[id];
                    return array.addComponent<T>(m_indexMaps[id][e], component);
                }

                /**
                 * @brief Allows for an entity `e` to access the same component data as entity `share`.
                 * 
                 * @tparam T The type of component pool to be accessed.
                 * @param e An `entity` made by the `createEntity` function. Will be granted access to the same component data as `share`.
                 * @param share An `entity` made by the `createEntity` function. Holds the index of the component to be shared.
                 */
                template<typename T>
                void share(entity e, entity share)
                {
                    uint32_t id = ComponentType<T>::id;
                    if(m_indexMaps[id][e] != -1)
                    {
                        removeComponent<T>(e);
                    }
                    m_indexMaps[id][e] = m_indexMaps[id][share];
                }

                /**
                 * @brief Retrieves data based off a provided data type and  `Entity`.
                 * 
                 * @details Accesses the proper component pool and retrieves a piece of data of type T
                 *          attached to the `entity` if such data exists.
                 * 
                 * @tparam T The type of component pool to be accessed and the type of component to be retrieved.
                 * @param e An `entity` made by the `createEntity` function. Used to index component data in respective pool.
                 * @return Returns the queried component (returns a reference if trivially copyable).
                 */
                template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
                T& getComponent(entity e)
                {
                    uint32_t id = ComponentType<T>::id;

                    #ifndef ECS_DEBUG_OFF
                        if(e == (entity)-1)
                            return m_componentArrays[id].getDefaultComponent<T>();
                    #endif

                    return m_componentArrays[id].getComponent<T>(m_indexMaps[id][e]);
                }

                template<typename T, typename = std::enable_if_t<!std::is_trivially_copyable<T>::value>>
                T getComponent(entity entity)
                {
                    uint32_t id = ComponentType<T>::id;

                    #ifndef ECS_DEBUG_OFF
                        if(entity == (entity)-1)
                            return m_componentArrays[id].getDefaultComponent<T>();
                    #endif

                    return m_componentArrays[id].getComponent<T>(m_indexMaps[id][entity]);
                }

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
                    uint32_t id = ComponentType<T>::id;
                    return m_componentArrays[id].getDefaultComponent<T>();
                }

                /**
                 * @brief Returns whether an `entity` has a component of the given type.
                 * 
                 * @tparam T The type of component that the `entity` is checked for.
                 * @param e The `entity` which is being checked.
                 * @return Returns true if the `entity` has the queried component type.
                 */
                template<typename T>
                bool containsComponent(entity e)
                {
                    uint32_t id = ComponentType<T>::id;
                    return (m_indexMaps[id][e] != -1);
                }

                /**
                 * @brief Removes component T attached to the given `entity`
                 * 
                 * @details Accesses the proper component pool, deletes, and deallocates a piece of data of type T attached to the 
                 * entity` if such data exists.
                 * 
                 * @tparam T The type of component pool to be accessed and the type of component to be deleted.
                 * @param entity An `Entity` made by the `createEntity` function. Used to index component data in respective pool.
                 * @return Returns a copy of the deleted component.
                 */
                template<typename T>
                T removeComponent(entity entity)
                {
                    T result = getComponent<T>(entity);

                    uint32_t id = ComponentType<T>::id;
                    size_t index = m_indexMaps[id][entity];

                    remove(id, index, entity);

                    return result;
                }
                
                /**
                 * @brief Sets the component attached to the given entity to a new value `update`.
                 * 
                 * @tparam T The type of component to be updated.
                 * @param e The entity whose component will be updated.
                 * @param update The data to replace the old data.
                 */
                template<typename T>
                void setComponent(entity e, const T& update)
                {
                    uint32_t id = ComponentType<T>::id;
                    size_t index = m_indexMaps[id][e];
                    size_t offset = m_componentArrays[id].setComponent<T>(index, update);

                    size_t size = m_indexMaps[id].size();
                    for(int i=0; i<size; i++)
                    {
                        if(m_indexMaps[id][i] > index)
                            m_indexMaps[id][i] += offset;
                    }
                }
                
                /**
                 * @brief Creates and returns a new 32-bit ID.
                 * 
                 * @details When a new `ComponentType<T>` is referenced, it is statically initialized with this function.
                 * 
                 * @tparam T The data type whose size is stored.
                 * @return A unique 32-bit ID.
                 */
                template<typename T>
                static const uint32_t newId()
                {
                    // whenever the compiler finds a new ComponentType, this function is called
                    uint32_t index = idCount;
                    spaceBuffer.push_back(sizeof(T));
                    complexBuffer.push_back(std::is_trivially_copyable<T>());
                    idCount++;
                    return index;
                }

                private:
                    /**
                     * @brief Removes a component with the given id attached to the given entity.
                     * 
                     * @param id The index of the component pool to be altered.
                     * @param index The `ComponentType` id of the component to be removed.
                     * @param entity The `entity` whose component will be removed.
                     */
                    void remove(uint32_t id, size_t index, entity e)
                    {
                        size_t offset = m_componentArrays[id].overwrite(index);
                        
                        m_indexMaps[id][e] = -1;

                        size_t size = m_indexMaps[id].size();
                        for(int i=0; i<size; i++)
                        {
                            if(m_indexMaps[id][i] > index && m_indexMaps[id][i] != (size_t)-1)
                            {
                                m_indexMaps[id][i] -= offset;
                            }
                        } 
                    }

                    static std::vector<size_t> spaceBuffer;        /** @brief A temporary vector that holds the size of each component per pool.*/
                    static std::vector<bool> complexBuffer;        /** @brief A temporary vector that stores whether a component type is copyable.*/

                    std::vector<ComponentArray> m_componentArrays; /** @brief A vector of component pools.*/
                    
                    std::vector<std::vector<size_t>> m_indexMaps;  /** @brief Maps an entity to the index of its components.*/
            };

            /**
             * @brief Holds relevant information for the `system` struct.
             * 
             * @details Prevents `system` from becoming bloated with information that is
             * rarely used; overall improves the cache safety of each `ecs`.
             */
            struct SystemSupplement
            {
                float priority;                                                        /** @brief Determines a `system`'s placement when running a function.*/
                uint32_t *index = NULL;                                                /** @brief A pointer to the `SystemType` id.*/

                std::vector<uint32_t> requirement;                                     /** @brief Holds the `ComponentType` ids for each of its required components.*/
                std::vector<size_t> indexMap;                                          /** @brief Maps an `entity` to its `system`.*/
                std::vector<entity> entities;                                          /** @brief All the entities attached to the `system`.*/
                void (*insertion)(entity, std::vector<entity>&, std::vector<size_t>&); /** @brief A user-defined function for inserting an entity into a `system`.*/

                /**
                 * @brief Constructor for `SystemSupplement`. Allocates space for current required `system`s.
                 * 
                 * @param numberOfEntities Describes how many entities have already been created.
                 */
                SystemSupplement(entity numberOfEntities)
                {
                    requirement = std::vector<uint32_t>();
                    indexMap = std::vector<size_t>(numberOfEntities);
                    entities = std::vector<entity>();
                    insertion = [] (entity e, std::vector<entity>& entities, std::vector<size_t>& map)
                    {
                        map[e] = entities.size();
                        entities.push_back(e);
                    };
                }

                /**
                 * @brief Inserts a given `entity` into a `system`.
                 * 
                 * @param e The `entity` to be inserted.
                 */
                void insert(entity e)
                {
                    insertion(e, entities, indexMap);
                }
            
                /**
                 * @brief Removes a given entity from a `system`.
                 * 
                 * @param e The `entity` to be removed.
                 */
                void extract(entity e)
                {
                    size_t revSize = entities.size();

                    entity last = entities[revSize-1];
                    entities[indexMap[e]] = last;
                    indexMap[last] = indexMap[e];

                    entities.pop_back();
                    indexMap[e] = -1;
                }
            };

            /**
             * @brief A manager for `system`s of every type.
             * 
             * @details Handles the organization of `system`'s and assigns `entity`s to their relevant `system`s based on their components.
             */
            struct SystemManager
            {
                static uint32_t idCount;      /** @brief Describes how many `SystemType`s have been created.*/
                static uint8_t functionIndex; /** @brief Describes how many different function types have been created.*/

                /**
                 * @brief Default constructor for `SystemManager`.
                 */
                SystemManager() {}

                /**
                 * @brief Constructor for `SystemManager`. Allocates space based on the amount of created `system`s and number of created `entity`s.
                 * 
                 * @param numberOfEntities The amount of `entity`s that have been created.
                 */
                SystemManager(entity numberOfEntities) : m_stores(idCount, system(functionIndex)), m_supplements(idCount, SystemSupplement(numberOfEntities)) {}

                /**
                 * @brief Runs user-defined functions of a certain type on every `system`.
                 * 
                 * @param container The `ecs` this manager is a variable of.
                 * @param index The index of the functions to be run.
                 */
                void runFunction(ecs& container, uint8_t index)
                {
                    for(system& store : m_stores)
                    {
                        if(store.initialized())
                        {
                            store.runFunction(container, index);
                        }
                    }
                }

                /**
                 * @brief Returns whether the given bitmap meets the requirements of the `system` at `index`.
                 * 
                 * @param index The index of the `system` whose requirements will be checked.
                 * @param bitmap The bitmap to be tested.
                 * @return Returns true if the bitmap matches.
                 */
                bool bitmapMatches(entity index, const std::vector<bool>& bitmap) const
                {
                    const std::vector<uint32_t>& requirement = m_supplements[index].requirement;
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

                /**
                 * @brief Returns whether the given bitmap meets the requirements of the `system` at `index`. The requirments must include the given bit.
                 * 
                 * @param index The index of the `system` whose requirements will be checked.
                 * @param bitmap The bitmap to be tested.
                 * @param bit The component type that must be included.
                 * @return Returns true if the bitmap matches.
                 */
                bool bitmapMatches(entity index, const std::vector<bool>& bitmap, uint32_t bit) const
                {
                    const std::vector<uint32_t>& requirement = m_supplements[index].requirement;
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

                /**
                 * @brief Sets a `system` to have a user-defined insertion for each `entity`.
                 * 
                 * @param index The index of the `system` to be changed.
                 * @param insert The new insertion function.
                 */
                void setInsertion(uint32_t index, void (*insert)(entity, std::vector<entity>&, std::vector<size_t>&))
                {
                    m_supplements[index].insertion = insert;
                }

                /**
                 * @brief Signals each `system` that a new `Entity` has been created.
                 */
                void addEntity()
                {
                    for(uint32_t i=0; i<idCount; i++)
                    {
                        m_supplements[i].indexMap.push_back(-1);
                    }
                }

                /**
                 * @brief Inserts a given `entity` into a `system`.
                 * 
                 * @param e The `entity` to be inserted.
                 * @param index The index of the `system` to be updated.
                 */
                void insertEntity(entity e, uint32_t index)
                {
                    m_supplements[index].insert(e);
                }

                /**
                 * @brief Removes a given `entity` from a `system`.
                 * 
                 * @param e The `entity` to be removed.
                 * @param index The index of the `system` to be updated.
                 */
                void extractEntity(entity e, const std::vector<bool>& bitmap)
                {
                    for(uint32_t i=0; i<idCount; i++)
                    {
                        if(bitmapMatches(i, bitmap))
                        {
                            m_supplements[i].extract(e);
                        }
                    }
                }

                /**
                 * @brief Signals each `system` that an `entity` has had a component removed.
                 * 
                 * @param e The `entity` whose component has been removed.
                 * @param bit The id of the type of the removed component,
                 * @param bitmap The new bitmap of the `entity`.
                 */
                void componentRemoved(entity e, uint32_t bit, const std::vector<bool>& bitmap)
                {
                    for(uint32_t i=0; i<idCount; i++)
                    {
                        if(bitmapMatches(i, bitmap, bit))
                        {
                            size_t revSize = m_supplements[i].entities.size();
                            entity last = m_supplements[i].entities[revSize-1];
                            m_supplements[i].entities[m_supplements[i].indexMap[e]] = last;
                            m_supplements[i].indexMap[last] = m_supplements[i].indexMap[e];

                            m_supplements[i].entities.pop_back();
                            m_supplements[i].indexMap[e] = -1;
                        }
                    }
                }

                /**
                 * @brief Checks whether a function type has been initialized.
                 * 
                 * @param index The index of the function type to be checked.
                 * @return Returns true if the function type exists.
                 */
                bool containsFunction(uint32_t index)
                {
                    return index < functionIndex;
                }

                /**
                 * @brief Returns whether a `system`'s static instance has been initialized.
                 * 
                 * @param index The index of the `system` to be checked.
                 * @return Return true if the `system`'s instance has been initialized.
                 */
                bool initialized(uint32_t index)
                {
                    return m_stores[index].initialized();
                }

                /**
                 * @brief Creates a new function that every `system` will be run through.
                 * 
                 * @return Returns the index to the created function type.
                 */
                uint8_t createSystemFunction()
                {
                    for(system& store : m_stores)
                    {
                        store.createFunction();
                    }
                    return functionIndex++;
                }

                /**
                 * @brief Updates the sizing of each `system` based on the amount `entity`s and function types created.
                 * 
                 * @param numberOfEntities Determines how much index space should be allocated for each `system`.
                 */
                void update(entity numberOfEntities)
                {
                    while(m_supplements.size() < idCount)
                    {
                        m_stores.push_back(system(functionIndex));
                        m_supplements.push_back(SystemSupplement(numberOfEntities));
                    }
                }

                /**
                 * @brief Creates a new `system` based on a given type T.
                 * 
                 * @tparam T The type of system to be created.
                 * @param instance The initial value for the created `system` to hold.
                 * @param priority The `system`s priority when a created function is run.
                 * @param id A reference to this `system`'s id; may be changed based on the `priority`.
                 * @return Returns a reference to the created `system`.
                 */
                template<typename T>
                system& createSystem(const T& instance, float priority, uint32_t& id)
                {
                    auto store = m_stores[id];
                    auto supplement = m_supplements[id];

                    uint32_t index = id/2;
                    uint32_t total = index, buffer;
                    if(id > 0)
                    {
                        do
                        {
                            buffer = index;
                            index = index/2;

                            if(m_supplements[total].priority > priority)
                            {
                                total -= (index+(buffer % 2));
                            }
                            else
                            {
                                total += (index+(buffer % 2));
                                if(!index)
                                {
                                    total += id%2;
                                }
                            }
                        }
                        while (index != 0);
                    }
                    
                    for(int i=id; i>total; i--)
                    {
                        moveTo(i-1, i);
                        *m_supplements[i].index = i;
                    }

                    system& system = m_stores[total];
                    m_supplements[total] = supplement;
                    m_supplements[total].priority = priority;
                    m_supplements[total].index = &id;

                    system = store;

                    id = total;

                    system.initialize<T>(instance);
                    return system;
                }

                /**
                 * @brief Get a reference to a `system` based on its `SystemType` id.
                 * 
                 * @param index The `SystemType` id based on this `system`'s associated data type.
                 * @return Returns a reference to the requested `system`.
                 */
                system& getSystem(uint32_t index)
                {
                    return m_stores[index];
                }

                /**
                 * @brief Returns the `entity` index map attached to the `system` associated with type T.
                 * 
                 * @tparam T The type of `system` whose index map will be accessed.
                 * @return Returns a reference to the requested index map.
                 */
                template<typename T>
                std::vector<size_t>& getIndexMap()
                {
                    uint32_t id = SystemType<T>::id;
                    return m_supplements[id].indexMap;
                }

                /**
                 * @brief Returns the `entity`s attaches to the `system` associated with type T.
                 * 
                 * @tparam T The type of `system` whose `entity`s will be accessed.
                 * @return Returns a reference to the requested `entity` list.
                 */
                template<typename T>
                std::vector<entity>& entities()
                {
                    uint32_t id = SystemType<T>::id;
                    return m_supplements[id].entities;
                }

                /**
                 * @brief Recursively adds each `ComponentType` from `Args` to the requirements for the `system` associated with type T.
                 * 
                 * @tparam T The type of `system` whose requirements will be set.
                 * @tparam Args The type of components to be added to this `system`s requirements.
                 */
                template<typename T, typename... Args>
                void addRequirements();

                /**
                 * @brief Returns the list of component requirements associated with the provided `SystemType` id.
                 * 
                 * @param index The `SystemType` id which indexes to the correct list of requirements.
                 * @return Returns a constant reference to the requirements list.
                 */
                const std::vector<uint32_t>& getRequirement(uint32_t index) const
                {
                    return m_supplements[index].requirement;
                }

                /**
                 * @brief Creates and returns a new 32-bit ID.
                 * 
                 * @details When a new `SystemType<T>` is referenced, it is statically initialized with this function.
                 * 
                 * @tparam T The data type whose size is stored.
                 * @return A unique 32-bit ID.
                 */
                template<typename T>
                static const uint32_t newId()
                {
                    uint32_t index = idCount;
                    // whenever the compiler finds a new ComponentType, this function is called
                    m_spaceBuffer.push_back(sizeof(T));
                    idCount++;
                    return index;
                }

                private:
                    static std::vector<size_t> m_spaceBuffer;    /** @brief A temporary vector that holds the size of each static `system` instance.*/

                    std::vector<system> m_stores;                /** @brief A vector of all the created `system`s.*/
                    std::vector<SystemSupplement> m_supplements; /** @brief A vector of supplemental `system` information that is not needed when each `system` is iterated over.*/

                    /**
                     * @brief Transfers all `system` data from one index to another.
                     * 
                     * @param from Original index of `system` data.
                     * @param to The location the `system` data will be sent to; must already be allocated.
                     */
                    void moveTo(uint32_t from, uint32_t to)
                    {
                        m_stores[to] = m_stores[from];
                        m_supplements[to] = m_supplements[from];
                    }

                    /**
                     * @brief Adds a single requirement to the `system` associated with type `Sys`.
                     * 
                     * @tparam S The component type which will be added to the requirements.
                     * @param id The index of the `system` whose requirements will be appended.
                     */
                    template<typename S>
                    void addRequirement(uint32_t id)
                    {
                        uint32_t componentId = ComponentType<S>::id;
                        std::vector<uint32_t>& requirement = m_supplements[id].requirement;

                        for(uint32_t index = 0; index < requirement.size(); index++)
                        {
                            if(requirement[index] == componentId)
                                return;
                        }
                        requirement.push_back(componentId);
                    }

                    /**
                     * @brief Recursively calls `addRequirement` for each provided type in `Args` until it is empty.
                     * 
                     * @tparam First The type to be added to a `system`'s requirements.
                     * @tparam Args The future types to be added to a `system`'s requirements.
                     * @param id The index of the `system` whose requirements will be appended.
                     */
                    template<typename First, typename... Args>
                    void addRequirementsRecursive(uint32_t id)
                    {
                        addRequirement<First>(id);
                        addRequirementsRecursive<Args...>(id);
                    }

                    /**
                     * @brief The end point of `addRequirementsRecursive`; called when `Args` is empty.
                     * 
                     * @tparam Args An empty list of types that have already been added to a `system`'s requirements.
                     * @param id The index of the `system` whose requirements have been altered.
                     */
                    template<typename... Args>
                    std::enable_if_t<sizeof...(Args) == 0> addRequirementsRecursive(uint32_t id) {}
            };


            /**
             * @brief A manager for created and destroyed `Entity` variables.
             * 
             * @details Handles the creation and destruction of unique `Entity` values. Also tends to each 
             * `entity`'s component bitmap.
             */
            struct EntityManager
            {
                /**
                 * @brief Creates an 'entity' with a unique value.
                 * 
                 * @details Either returns an `entity` with an incremented value, or recycles an `entity` that 
                 * has been deleted. Also initializes the `entity`'s bitmap.
                 * 
                 * @return Returns a newly created `entity`. 
                 */
                entity createEntity()
                {
                    entity entity = m_entityCount++;

                    // recycles any entities that have been destroyed
                    if(!m_removedEntities.empty())
                    {
                        entity = m_removedEntities.back();
                        m_removedEntities.pop_back();
                    }
                    // creates a new bitmap if no entities can be recycled
                    else
                    {
                        m_componentBitmaps.push_back(std::vector<bool>(ComponentManager::idCount + 1));
                    }
                    m_componentBitmaps[entity].back() = true;
                    return entity;
                }

                /**
                 * @brief Removes and recycles an 'entity'.
                 * 
                 * @details Stores the deleted `entity` in an internal vector. Resets that `entity`'s bitmap.
                 * 
                 * @param e An `entity` created by the `createEntity` function.
                 */
                void removeEntity(entity e)
                {
                    // resets the bitmap to be recycled
                    m_componentBitmaps[e] = std::vector<bool>(ComponentManager::idCount + 1);
                    m_removedEntities.push_back(e);
                    m_entityCount--;
                }

                /**
                 * @brief Sets the bit in the bitmap of a certain `entity` at a defined index.
                 * 
                 * @param e An `entity` created by the `createEntity` function.
                 * @param index  The index of the bit to set.
                 * @param bit    The value the bit will be set to.
                 */
                void setComponentBit(entity e, uint32_t index, bool bit)
                {
                    m_componentBitmaps[e][index] = bit;
                }

                /**
                 * @brief Returns the bitmap of a certain `entity`.
                 * 
                 * @param e An `entity` created by the `createEntity` function.
                 * @return The component bitmap attached to the given `entity`.
                 */
                std::vector<bool>& getBitmap(entity e)
                {
                    return m_componentBitmaps[e];
                }

                /**
                 * @brief Determines whether the given `entity` has been removed or is still active.
                 * 
                 * @param e An `entity` created by the `createEntity` function.
                 * @return The `active` state of an `entity`.
                 */
                bool entityActive(entity e) const
                {
                    return m_componentBitmaps[e].back();
                }

                /**
                 * @brief Returns whether the provided `entity` has been created by this `EntityManager`.
                 * 
                 * @param e The `entity` to be tested.
                 * @return Returns true if the `entity` was created with this manager.
                 */
                bool contains(entity e) const
                {
                    return totalEntityCount() > e;
                }

                /**
                 * @brief Returns the number of non-deleted `entity`s that have been created.
                 *
                 * @return The number of active entities.
                 */
                entity activeEntityCount() const
                {
                    return m_entityCount;
                }

                /**
                 * @brief Returns the number of created `entity`s (even if they have been deleted).
                 * 
                 * @return The total number of entities (active and removed).
                 */
                entity totalEntityCount() const
                {
                    return m_entityCount + m_removedEntities.size();
                }

                private:
                    entity m_entityCount = 0;                          /** @brief The total number of active entities.*/
                    std::vector<entity> m_removedEntities;             /** @brief A list of every entity that has been removed.*/
                    std::vector<std::vector<bool>> m_componentBitmaps; /** @brief The component bitmaps corresponding to each entity.*/
            };


            EntityManager m_entityManager;       /** @brief Handles entity creation/destruction.*/
            ComponentManager m_componentManager; /** @brief Handles component creation/destruction.*/
            SystemManager m_systemManager;       /** @brief Handles system creation and entity distribution.*/
    };

    /**
     * @brief Creates a unique id for any data type that is made into a component.
     * 
     * @tparam T The type which is assigned an id.
     */
    template <typename T>
    inline const uint32_t ComponentType<T>::id = ecs::newComponentId<T>();

    /**
     * @brief Creates a unique id for any data type that is made into a system.
     * 
     * @tparam T The type which is assigned an id.
     */
    template <typename T>
    inline uint32_t SystemType<T>::id = ecs::newSystemId<T>();

    template<typename T, typename>
    T& ecs::ComponentArray::addComponent(size_t& index, const T& component)
    {
        #ifndef ECS_DEBUG_OFF
            if(index != (size_t)-1)
            {
                ecs::error = 1;
                return *reinterpret_cast<T*>(&m_components[index]);
            }
        #endif

        size_t arraySize = m_components.size();

        // copy data from `&component` to the newly allocated space

        m_components.resize(arraySize + sizeof(T));
        object::serialize(component, m_components, arraySize);

        // save the index for the entity
        index = arraySize;

        // could use `getComponent<T>()`, this avoids unnecessary index check
        return *reinterpret_cast<T*>(&m_components[arraySize]);
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

        size_t arraySize = m_components.size();

        // copy data from `&component` to the newly allocated space
        size_t length = Serialization<T>::length(component);

        m_components.resize(arraySize + sizeof(size_t) + length);
        object::serialize(component, m_components, arraySize, length);

        // save the index for the entity
        index = arraySize;

        // could use `getComponent<T>()`, this avoids unnecessary index check
        return object::deserialize<T>(m_components, index);
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
        return object::deserialize<T>(m_components, index);
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
        return object::deserialize<T>(m_components, index);
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
        size_t offset = object::resize<T>(length, m_components, index);
        object::serialize<T>(update, m_components, index, length);
        return offset;
    }

    template<typename T, typename... Args>
    void ecs::SystemManager::addRequirements()
    {
        uint32_t id = SystemType<T>::id;
        addRequirementsRecursive<Args...>(id);
    }
}

/**
 * @brief Defines the serialization process for a `std::string`
 */
template <>
struct Serialization<std::string>
{
    static size_t length(const std::string& data)
    {        
        return data.size()*sizeof(char) + sizeof(size_t);
    }

    static size_t serialize(const std::string& value, std::vector<uint8_t>& stream, size_t index)
    {
        size_t stringLength = value.size();
        size_t sizing = sizeof(size_t);
        size_t count = 0;

        std::memcpy(&stream[index], &stringLength, sizing);
        for(size_t i = 0; i<stringLength; i++)
        {
            auto v = value[i];
            count += object::serialize<char>(v, stream, index + sizing + count);
        }
        return count + 2*sizing;
    }

    static std::string deserialize(std::vector<uint8_t>& stream, size_t index)
    {
        size_t size = object::deserialize<size_t>(stream, index);
        size_t offset = sizeof(size_t);
        size_t count = 0;

        std::string result = "";

        for(int i=0; i<size; i++)
        {
            auto v = object::deserialize<char>(stream, index + offset + count);
            result += v;
            count += object::length<char>(v);
        }

        return result;
    }
};

/**
 * @brief Defines the serialization process for a `std::vector<T>`
 * 
 * @tparam T The data type this `std::vector` holds.
 */
template<typename T>
struct Serialization<std::vector<T>>
{
    static size_t length(const std::vector<T>& data)
    {
        size_t result = 0;
        size_t length = data.size();
        for(int i=0; i<length; i++)
        {
            result += object::length(data[i]);
        }
        
        return result + sizeof(size_t);
    }

    static size_t serialize(const std::vector<T>& value, std::vector<uint8_t>& stream, size_t index)
    {
        size_t vectorLength = value.size();
        size_t sizing = sizeof(size_t);
        size_t count = 0;

        std::memcpy(&stream[index], &vectorLength, sizing);
        for(size_t i = 0; i<vectorLength; i++)
        {
            auto variable = value[i];

            size_t length = object::length<T>(variable);
            object::serialize<T>(variable, stream, index + sizing + count, length);
            count += length;
        }
        return count + sizing;
    }

    static std::vector<T> deserialize(std::vector<uint8_t>& stream, size_t index)
    {
        size_t size = object::deserialize<size_t>(stream, index);
        size_t offset = sizeof(size_t);
        size_t count = 0;

        std::vector<T> result = std::vector<T>(size);

        for(int i=0; i<size; i++)
        {
            result[i] = object::deserialize<T>(stream, index + offset + count);
            count += object::length<T>(result[i]);
        }

        return result;
    }
};

#endif