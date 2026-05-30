#ifndef LM_OBSERVER_H
#define LM_OBSERVER_H

class LMObserver
{
public:
    // A virtual destructor is mandatory for pure virtual C++ classes!
    virtual ~LMObserver() = default;

    // The event the GUI must implement.
    // We pass the bounding box of the changes, not the pixels themselves.
    virtual void onRegionChanged(int x, int y, int width, int height) = 0;
};

#endif // LM_OBSERVER_H