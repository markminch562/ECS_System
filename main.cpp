#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <stack>
#include <bitset>
struct vec3
{
    float x;
    float y;
    float z;
    ~vec3()
    {
        std::cout<<"getting rid of vec 3 data"<<std::endl;
        std::cout<<x<<" "<<y<<" "<<z<<std::endl;
    }
};
struct Object
{
    vec3 pos;
    vec3 rotation;
    char name[16];
};
static unsigned int BitTypes;
//we need a global queue that we can take information from to get out data out of

namespace MemorySystem {
    typedef std::bitset<64> bits;
    static int MaxSize = 0;
    int run() {
        static int var = 0;
        MaxSize += 1;
        return var++;
    }
    template<typename T>
    struct Pool
    {
        std::vector<T> set;
        void print()
        {
        }
    };

    #define MaxEntities 2000

    template<typename T>
    struct ComponentItem
    {
        inline ComponentItem()
        {

            //The Component and the Entity_LUT Both Start with 20 spaces to prevent slow pushbacks when adding and searching for data
            //While hopefully keeping size down
            //If you need to add more data from the beginning you can  use  the overload function version
            ComponentData(20);
            Entity_LUT = std::vector<unsigned int> (20, MaxEntities+1);
            Component_LUT = std::vector<unsigned int> (MaxEntities, MaxEntities+1);
            CurrentCapacity = ComponentData.capacity();
            CurrentSize = 0;
        }
        ComponentItem(unsigned int size)
        {
            ComponentData(size);
            Entity_LUT = std::vector<unsigned int> (size, MaxEntities+1);
            Component_LUT = std::vector<unsigned int> (MaxEntities,MaxEntities+1);
            CurrentCapacity = ComponentData.capacity();
            CurrentSize = 0;
        }
        inline void push_back(T item, unsigned int entity)
        {
            if(CurrentCapacity == CurrentSize)
            {
                CurrentCapacity = CurrentCapacity*GrowthFactor;
                ComponentData.resize(CurrentCapacity);
                Entity_LUT.resize(CurrentCapacity);
            }

                ComponentData[CurrentSize] = item;
                Entity_LUT[CurrentSize] = entity;
                Component_LUT[entity] = CurrentSize;
                CurrentSize++;
        }
        inline void Remove(unsigned int entity)
        {
            unsigned int ComponentLocation = Component_LUT[entity];
            //check for edge cases that where user passes wrong entity or invalid data access error
            if(ComponentLocation == MaxEntities+1 || ComponentLocation > CurrentSize-1) return;

            //If the value we are removing is at the very back we can skip these steps
            if(ComponentLocation < CurrentSize-1)
            {
                //copy the data that is in the back up to where the current entitys component is located
                ComponentData[ComponentLocation] = ComponentData[CurrentSize-1];
                unsigned int previousEntityLocation = Entity_LUT[CurrentSize-1];
                Entity_LUT[ComponentLocation] = previousEntityLocation;
                Component_LUT[previousEntityLocation] = ComponentLocation;
            }
            //No need to delete data from the component just make it un accessable from the view of the system
            Component_LUT[entity] = MaxEntities+1;
            Entity_LUT[CurrentSize-1] = MaxEntities+1;
            CurrentSize--;
        }

        inline T* getComponentP(unsigned int entity)
        {
            unsigned int Location = Component_LUT[entity];
            if(Location == MaxEntities+1 || Location > CurrentSize-1) return nullptr;
            return &ComponentData[Location];
        }
        inline T getComponent(unsigned int entity)
        {
            unsigned int Location = Component_LUT[entity];
            if(Location == MaxEntities+1 || Location > CurrentSize-1)
            {
                std::cout<<"ERROR THERE IS NO COMPONENT FOUND AT THIS LOCATION"<<std::endl;
                return ComponentData[0];
            }
            return ComponentData[Location];
        }

        inline std::vector<T> getComponentArray(std::vector<unsigned int> entities)
        {
            std::vector<T> componentList;
            for(int i = 0; i < entities.size(); i++)
            {
                unsigned int Location = Component_LUT[entities[i]];
                if (Location != MaxEntities + 1 && Location < CurrentSize)
                {
                    componentList.push_back(ComponentData[Location]);
                }
            }
            return componentList;
        }
        unsigned int CurrentSize = 0;
        unsigned int CurrentCapacity;
        //GrowthFactor lets the user limit the rate a which individual vectors in the component storage system grow
        //This can be useful with Components that take up a lot of space and would take a lage amount of time for the vector to
        //resize
        unsigned int GrowthFactor = 2;

        //These are the main storage systems for the system
        //Component Data is a Dense Set of our actual data that is being stored
        std::vector<T> ComponentData;
        //Entity_LUT is a Look Up Table that allow us to find the Entity given a Component index value
        std::vector<unsigned int> Entity_LUT;
        //Component_LUT is a Look Up Table the allows us to Find where a Component is given an Entity value
        std::vector<unsigned int> Component_LUT;
    };






    template<typename T>
    void FreeMemory(int index, void* Sets);

    template<typename T>
    class TypeBitSet {
        inline static const int Family = run();
        inline static const bits BittType = 0 << run();
    public:
        static int type() {
            return Family;
        }
        static bits BitType()
        {
            return BittType;
        }

    };

    struct EntityManager
    {
        EntityManager()
        {
            for(unsigned int i = MaxEntities-1; i>-1; i--)
            {
                EntityPool.push(i);
            }
        }

        unsigned int CreateEntity()
        {
            if(EntityPool.size() != 0) {
                unsigned int newEntt = EntityPool.top();
                ActiveEntities.push_back({newEntt, bits()});
                EntityPool.pop();
                return newEntt;
            }
            else
            {
                return MaxEntities;
            }
        }
        void DeleteEntity(unsigned int oldEntt)
        {
            size_t ActiveSize = ActiveEntities.size();
            for(int i = 0; i <ActiveSize; i++)
            {
                if(ActiveEntities[i].first == oldEntt)
                {
                    if(ActiveSize-1 == i)
                    {
                        ActiveEntities.pop_back();
                    }
                    else
                    {
                        ActiveEntities[i] = ActiveEntities[ActiveSize-1];
                        ActiveEntities.pop_back();
                    }
                }
            }
        }
        //Future add a clone Entity function to the system as well


        //By default the ActiveEntities will be empty when a EntityManager Context is started
        //And the Entity Pool Stack will be filled from bottom to top
        //When an entity is deleted it will be put back on top so that it can be used again sooner
        //which should reduce the number of items that need to be searched in some senarios
        std::vector<std::pair<unsigned int, bits>> ActiveEntities;
        std::stack<unsigned int> EntityPool;
    };



    struct Manager
    {
        std::vector<void*> dataSets;
        std::vector<void (*)(int, void *)> CleanupFunctions;
        std::vector<int> typeList;
        std::vector<int> DenseIndex;

        //The Manager has its own Entity Manager that controls all of the Entitys that have been created
        EntityManager EnttSystem;


        Manager()
        {
            typeList.resize(MaxSize,-1);
        }
        /*
         * The Destructor for the Manager goes through the vector of clean up functions and removes all of the data
         * from the dense dataset vector so that all of the data gets safely released from memory
         * This is done
         */
        inline ~Manager()
        {
           for(int i = 0; i < dataSets.size(); i++)
           {
               CleanupFunctions[i](i,&dataSets);
           }
        }
        template<typename T>
        inline  void CreatePool()
        {
            //Pool<T>* pool_data = new Pool<T>;
            if(typeList[TypeBitSet<T>::type()] != -1)
            {
                std::cout<<"ERROR THIS DATA SET ALREADY EXIST IN THIS SCOPE"<<std::endl;
            }
            else
            {
                //The cleanup memory function and  the pointers to the data should exist as alligned
                //Dense Sets at all time
                dataSets.push_back(new Pool<T>());
                CleanupFunctions.push_back(&FreeMemory<T>);
                /*
                if(TypeBitSet<T>::type() > typeList.size())
                {
                    typeList.resize(TypeBitSet<T>::type()+1);
                    typeList.push_back(-1);
                }
                if( dataSets.size() > typeList.size())
                {
                    typeList.resize(dataSets.size()+10);
                    typeList.push_back(-1);
                }
                 */
                //The Type List is a sparse set and points to where the object of that type exist in the dense set
                typeList[TypeBitSet<T>::type()] = dataSets.size() - 1;
                DenseIndex.push_back(TypeBitSet<T>::type());
            }
        }

        template <typename T>
        inline void RemovePool()
        {

            int indexPos = typeList[TypeBitSet<T>::type()];
            int back = dataSets.size()-1;

            if(indexPos != -1) {
                //remove the pool from the dense set
                CleanupFunctions[indexPos](indexPos, &dataSets);
                //move new vector into the dense spot;
                if (indexPos != back) {
                    dataSets[indexPos] = dataSets[back];
                    CleanupFunctions[indexPos] = CleanupFunctions[back];
                }
                dataSets.pop_back();
                CleanupFunctions.pop_back();
                typeList[indexPos] = -1;
            }
        }
        template<typename T>
        inline void push_back(T newData)
        {
            void* dataAliase = dataSets[typeList[TypeBitSet<T>::type()]];
            Pool<T>* data = static_cast<Pool<T>*>(dataAliase);
            data->set.push_back(newData);
        }

        template<typename T>
        inline void print_pool()
        {
            void* dataAliase = dataSets[typeList[TypeBitSet<T>::type()]];
            Pool<T>* data = static_cast<Pool<T>*>(dataAliase);
            data->print();
        }

        //create a callback queue for cleaning up my variables

    };




    template<typename T>
    inline void FreeMemory(int index, void* Sets)
    {
        Manager* dataSets = static_cast<Manager*>(Sets);
        //delete line below latter
        Pool<T>* tempCheck = (Pool<T>*)dataSets->dataSets[index];
        //
        //std::cout<<"Removing item from index "<<index<<std::endl;
        delete (Pool<T>*)dataSets->dataSets[index];
    }
}

int main() {

    {
        MemorySystem::Manager manage;
        manage.CreatePool<int>();
        manage.CreatePool<float>();
        manage.CreatePool<vec3>();

        manage.push_back<vec3>({2.0, 1.5, 1.7});
        manage.push_back<int>(1);
        manage.push_back<float>(2.03);

        manage.print_pool<vec3>();
        manage.print_pool<float>();
        std::cout<<"Removing the pool"<<std::endl;
        //manage.RemovePool<vec3>();
        std::cout<<"print int"<<std::endl;
        manage.print_pool<int>();
    }
    std::cout<<"This is printed after the data has been deleted did it work"<<std::endl;

return 0;
}
