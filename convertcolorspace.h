#ifndef CONVERTCOLORSPACE_H
#define CONVERTCOLORSPACE_H

#include <memory>

struct Data;

std::unique_ptr<Data> rgb32toyuv400p(std::unique_ptr<Data> frame);
std::unique_ptr<Data> rgb32toyuv420p(std::unique_ptr<Data> frame);

#endif // CONVERTCOLORSPACE_H
