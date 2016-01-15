// gol.cpp : Defines the entry point for the console application.
//

#include <vector>
#include <memory>
#include <map>
#include <assert.h>
#include <iostream>
#include <locale>

using namespace std;

typedef unsigned long Color;
typedef std::map<int, Color> CellsColor;

class Cell
{
public:
    explicit Cell()
    {
    }

    bool alive() const
    {
        return false;
    }

    void toggleLife()
    {
        isAlive = !isAlive;
    }

    int r() const
    {
        return (color >> 16 & 0xFF);
    }
    int g() const
    {
        return (color >> 8 & 0xFF);
    }
    int b() const
    {
        return (color & 0xFF);
    }

    int id = 0;
    Color color = 0x0;

private:
    bool isAlive = false;
};

typedef Cell* CellGrid;

struct CountAndColor
{
    int count;
    Color color;

    CountAndColor(int count, Color color)
    {
        this->count = count;
        this->color = color;
    }
};

class Environment
{
private:
    CellGrid _Cells;
    CellGrid _OldCells;
    shared_ptr<CellsColor> _CellsBorn;
    shared_ptr<CellsColor> _CellsDied;

    shared_ptr<CellsColor> _LiveCells;
    int _Rows = 0;
    int _Columns = 0;
    int _GenerationNumber = 0;

    const Color deadColor = 0xeeeeee;

public:
    explicit Environment(int rows, int columns)
        :
        _Rows(rows),
        _Columns(columns)
    {
        _Cells = new Cell[rows * columns];
        _OldCells = nullptr;

        for (auto i = 0; i < _Columns * _Rows; i++)
        {
            _Cells[i].id = i;
        }

        _CellsBorn = make_shared<CellsColor>();
        _CellsDied = make_shared<CellsColor>();
        _LiveCells = make_shared<CellsColor>();
    }

    Cell* getCell(int x, int y) const
    {
        return &_Cells[x * _Columns + y];
    }

    Cell* getOldCell(int x, int y) const
    {
        return &_OldCells[x * _Columns + y];
    }

    void NextGeneration()
    {
        _GenerationNumber++;
        _CellsBorn->clear();
        _CellsDied->clear();

        _OldCells = new Cell[_Columns * _Rows];
        memcpy(_OldCells, _Cells, _Columns * _Rows * sizeof(Cell));

        for (auto j = 0; j < _Rows; j++)
        {
            for (auto i = 0; i < _Columns; i++)
            {
                auto currCell = getOldCell(i, j);
                auto alive = currCell->alive();
                CountAndColor neighbor = this->neighborCountAndAverageColor(i, j);
                auto averageColor = neighbor.color;
                if (alive && (neighbor.count < 2 || neighbor.count > 3))
                {
                    setColorAndFlipCell(i, j, deadColor);
                    _LiveCells->erase(currCell->id);
                    //passing 0 as value because the cell id passed as the key expresses enough information
                    _CellsDied->insert_or_assign(currCell->id, 0x000000);
                }
                else if (!alive && neighbor.count == 3)
                {
                    setColorAndFlipCell(i, j, averageColor);
                    //the client only needs to know the color
                    _LiveCells->insert_or_assign(currCell->id, averageColor);
                    _CellsBorn->_Insert_or_assign(currCell->id, averageColor);
                }
            }
        }

        delete _OldCells;

    }

    void setColorAndFlipCell(int x, int y, Color color)
    {
        this->setCellColor(x, y, color);
        this->flipCell(x, y);
    }

    void setCellColor(int x, int y, Color color)
    {
        assert(!(x < 0 || x >= _Columns || y < 0 || y >= _Rows));

        getCell(x, y)->color = color;
    }

    void flipCell(int x, int y)
    {
        assert(!(x < 0 || x >= _Columns || y < 0 || y >= _Rows));

        auto cell = getCell(x, y);
        auto wasAlive = cell->alive();
        cell->toggleLife();
        if (wasAlive) {
            _LiveCells->erase(cell->id);
        }
        else {
            _LiveCells->insert_or_assign(cell->id, cell->color);
        }
    }

    void ApplyNeighborValues(int x, int y, int& count, int& averageR, int& averageG, int& averageB)
    {
        auto cell = getOldCell(x, y);
        if (cell->alive())
        {
            count++;
            averageR += cell->r();
            averageG += cell->g();
            averageB += cell->b();
        }
    }

    CountAndColor neighborCountAndAverageColor(int x, int y)
    {
        assert(!(x < 0 || x >= _Columns || y < 0 || y >= _Rows));

        auto count = 0;
        auto averageR = 0;
        auto averageG = 0;
        auto averageB = 0;

        if (x > 0)
        {
            ApplyNeighborValues(x - 1, y, count, averageR, averageG, averageB);
        }
        else if (x < _Columns - 1)
        {
            ApplyNeighborValues(x + 1, y, count, averageR, averageG, averageB);
        }
        else if (y > 0)
        {
            ApplyNeighborValues(x, y - 1, count, averageR, averageG, averageB);
        }
        else if (x < _Rows - 1)
        {
            ApplyNeighborValues(x, y + 1, count, averageR, averageG, averageB);
        }

        if (getCell(x, y)->alive()) {
            count--;
        }

        if (count > 0)
        {
            averageR = averageR / count;
            averageG = averageG / count;
            averageB = averageB / count;
        }
        Color color = createRGB(averageR, averageG, averageB);

        CountAndColor results(count, color);
        return results;
    }

    unsigned long createRGB(int r, int g, int b) const
    {
        return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
    }

    int getGen() const
    {
        return _GenerationNumber;
    }
};

int main()
{
    const auto begin_time = clock();
    
    auto env = std::make_shared<Environment>(40, 40);

    for (auto i = 0; i < 40; i++)
        for (auto j = 0; j < 40; j++)
        {
            if (i * j % 5 == 0)
                env->getCell(i, j)->toggleLife();
        }

    for (auto x = 0; x < 10000; x++)
    {
        env->NextGeneration();
        if (env->getGen() % 1000 == 0) cout << "generation: " << env->getGen() << endl;
    }

    // do something
    std::cout << float(clock() - begin_time) / CLOCKS_PER_SEC;

    char empty;
    cin >> empty;

    return 0;
}

