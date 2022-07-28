//
// Created by Paul Walker on 5/30/22.
//

#ifndef SST_JUCEGUI_DISCRETE_H
#define SST_JUCEGUI_DISCRETE_H

#include <string>
#include "Labeled.h"

namespace sst::jucegui::data
{
struct Discrete : public Labeled
{
    virtual ~Discrete() = default;

    struct DataListener
    {
        virtual ~DataListener() = default;
        // FIXME - in the future we may want this more fine grained
        virtual void dataChanged() = 0;
    };
    void addGUIDataListener(DataListener *l) { guilisteners.insert(l); }
    void removeGUIDataListener(DataListener *l) { guilisteners.erase(l); }
    void addModelDataListener(DataListener *l) { modellisteners.insert(l); }
    void removeModelDataListener(DataListener *l) { modellisteners.erase(l); }

    virtual int getValue() const = 0;
    virtual void setValueFromGUI(const int &f) = 0;
    virtual void setValueFromModel(const int &f) = 0;

    virtual std::string getValueAsString() const { return std::to_string(getValue()); }
    virtual void setValueAsString(const std::string &s) { setValueFromGUI(std::atof(s.c_str())); }

    virtual int getMin() const { return 0; }
    virtual int getMax() const { return 1; }

    virtual void jog(int dir)
    {
        int v = getValue() + dir;
        if (v < getMin())
            v = getMax();
        if (v > getMax())
            v = getMin();
        setValueFromGUI(v);
    }

  protected:
    std::unordered_set<DataListener *> guilisteners, modellisteners;
};

struct BinaryDiscrete : public Discrete
{
    int getMin() const override { return 0; }
    int getMax() const override { return 1; }
};
} // namespace sst::jucegui::data
#endif // SST_JUCEGUI_DISCRETE_H