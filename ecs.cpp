#include "ecs.h"

#include <iostream>


uint16_t object::ecs::error = 0;

uint32_t object::ecs::SystemManager::idCount = 0;
uint8_t object::ecs::SystemManager::functionIndex = 0;

uint32_t object::ecs::ComponentManager::idCount = 0;

std::vector<size_t> object::ecs::SystemManager::m_spaceBuffer = {};
std::vector<size_t> object::ecs::ComponentManager::spaceBuffer = {};
std::vector<bool> object::ecs::ComponentManager::complexBuffer = {};

void printSerializationError(const std::string& message)
{
    std::cout << message << '\n';
}

void object::ecs::parseError()
{
    uint16_t err = getError();
    switch(err)
    {
        case 0:
            std::cout << "N/A.\n";
        break;
        case 1:
            std::cout << "ERROR :: Entity already contains component; Call to `addComponent` failed.\n";
        break;
        case 2:
            std::cout << "ERROR :: Component could not be accessed from call to `getComponent`.\n";
        break;
        case 3:
            std::cout << "ERROR :: Component was not present from call to `removeComponent`.\n";
        break;
        case 4:
            std::cout << "ERROR :: Entity has been deleted; Call to `addComponent` failed.\n";
        break;
        case 5:
            std::cout << "ERROR :: System does not exist.\n";
        break;
        case 6:
            std::cout << "ERROR :: Entity does not exist.\n";
        break;
    }
}