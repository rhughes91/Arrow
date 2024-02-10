#include <fstream>
#include <iostream>
#include "ecs.h"


struct Renderer
{
    uint32_t test = 102;
};

struct Manager
{
    double test = 43.258;
};

struct Vector3
{
    uint32_t x, y, z;
};

struct Transform
{
    Vector3 position, scale;
};


void testingGround(object::ecs& container, uint32_t numberOfEntities)
{
    for(int i=0; i<=numberOfEntities; i++)
    {
        entity e = container.createEntity();
        container.addComponent<Transform>(e, {{1, 2, 3}, {4, 5, 6}});
        container.addComponent<float>(e, i);
        if(i % 2 == 0)
        {
            container.addComponent<std::vector<int>>(e, {(int)e});
        }
        else
        {
            container.addComponent<double>(e);
        }
    }
}

void ecsTest(uint32_t number)
{
    object::ecs test;
    uint8_t UPDATE = test.createSystemFunction();

    auto& renderer = test.createSystem<Renderer, float, std::vector<int>, Transform>({4}, -100);
    renderer.setFunction(UPDATE, []
    (object::ecs& container, object::ecs::system& system)
    {
        Renderer renderer = system.getInstance<Renderer>();

        for(entity entity : container.entities<Renderer>())
        {
            std::vector<int> one = container.getComponent<std::vector<int>>(entity);
            float& two = container.getComponent<float>(entity);
            Transform& three = container.getComponent<Transform>(entity);

            std::cout << one[0] << '\n';
        }
    });

    auto& manager = test.createSystem<Manager, float, Transform>();
    manager.setFunction(UPDATE, []
    (object::ecs& container, object::ecs::system& system)
    {
        Manager& manager = system.getInstance<Manager>();

        for(entity entity : container.entities<Manager>())
        {
            // double& one = container.getComponent<double>(entity);
            float& two = container.getComponent<float>(entity);
            Transform& three = container.getComponent<Transform>(entity);
        }
    });

    testingGround(test, number);
    test.run(UPDATE);

    test.parseError();
}

int main(int argc, char *argv[])
{
    uint32_t number = 100;
    if(argc > 1)
    {
        number = strtoul(argv[1], NULL, 0);
    }

    ecsTest(number);
    ecsTest(number);
}