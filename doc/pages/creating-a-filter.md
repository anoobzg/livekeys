# Creating a filter

The [previous section]() described the process of configuring and creating
a simple QML item using Livekeyss functionality. This part will dive into Livekeys a bit deeper, as it will describe
how to display a processed matrix on screen and how to create a filter which does **input to output** processing.

A QML item with a displayable matrix property (for example an item that loads an image from the hard drive, or creates
a new processed image from an input) is an item that internally inherits [QMatDisplay]()
functionality. QMatDisplay is actually an implementation of QQuickItem that contains a displayable QMat property
called **output**. All items that inherit QMatDisplay can work with this property in order to display their processed
result.

A simple example is the QTonemap class which works directly on the output property. Here's a snippet from
its process method:

```
void QTonemap::process(){
    if ( m_tonemapper && !m_input->data().empty() && isComponentComplete() ){
        m_tonemapper->process(m_input->data(), m_store);
        m_store.convertTo(*output()->cvMat(), CV_8UC3, 255);
        setImplicitWidth(output()->data().cols);
        setImplicitHeight(output()->data().rows);
        emit outputChanged();
        update();
    }
}
```

After the processing from the tonemapper, the matrix is used to display the element on screen.

In addition to QMatDisplay's functionality, the QMatFilter adds an **input** property and a function that will be used
by derived classes to implement the actual filtering.

## A filter example.

This is an example of a simple filter that converts an image to grayscale. Open CV assertions and other error handling
issues are solved by `QMatFilter`, so they do not need to be handled by our transform function.

```
#include "QMatFilter.h"
#include "opencv2/imgproc/imgproc.hpp"

class QBGRToGray : public QMatFilter{

    Q_OBJECT

public:
    explicit QBGRToGray(QQuickItem *parent = 0) : QMatFilter(parent){}

    virtual void transform(cv::Mat &in, cv::Mat &out){
        cv::cvtColor(in, out, CV_BGR2GRAY);
    }

};
```

The transform function gets called whenever the [QMatFilter::input]() element changes. The output
property changes are signaled automatically after the transform function is finished processing.


## Implementing a parameter based filter.

Open CV functions that have more parameters rather than a single input and output require us to set
qml properties for each. Every time the property changes, we signal the QMatFilter that a transformation
is required.


For this example, we will be implementing the addWeighted function from Open CV in order to show how
qml properties are implemented.

_This example is available under live-tutorial repository, in [plugins/tutorial/src](https://github.com/live-keys/live-tutorial)
if you want to just see the end result._

The addWeighted function has the following parameters :

* **src1** First input array.
* **alpha** Weight of the first array elements.
* **src2** Second input array of the same size and channel number as src1.
* **beta** Weight of the second array elements.
* **dst** Output array that has the same size and number of channels as the input arrays.
* **gamma** Scalar added to each sum.

**src1** and **dst** will be the actual input and output properties, so we need to add the **src2** as
another QMat property, and also the **alpha**, **beta** and **gamma** as *real* type properties.

We can use the previously created **tutorial** plugin and add a new element. Create a new class called **AddWeighted**.
Include and extend QMatFilter, and add the following properties :

```
Q_PROPERTY(QMat* input2 READ input2 WRITE setInput2 NOTIFY input2Changed)
Q_PROPERTY(qreal alpha  READ alpha  WRITE setAlpha  NOTIFY alphaChanged)
Q_PROPERTY(qreal beta   READ beta   WRITE setBeta   NOTIFY betaChanged)
Q_PROPERTY(qreal gamma  READ gamma  WRITE setGamma  NOTIFY gammaChanged)
```

Add the setters and getters, and implement the following function :

```
// addweighted.h
void transform(cv::Mat& in, cv::Mat& out);
```

```
// addweighted.cpp

void AddWeighted::transform(cv::Mat& in, cv::Mat& out){
    cv::Mat* in2 = m_input2->cvMat();
    if ( in.size() == in2->size() && in.channels() == in2->channels() ){
        cv::addWeighted(in, m_alpha, *in2, m_beta, m_gamma, out);
    }
}
```

The final version will look like the following :

```
// addweighted.h

#include "qmatfilter.h"

class AddWeighted : public QMatFilter{

    Q_OBJECT
    Q_PROPERTY(QMat* input2 READ input2 WRITE setInput2 NOTIFY input2Changed)
    Q_PROPERTY(qreal alpha  READ alpha  WRITE setAlpha  NOTIFY alphaChanged)
    Q_PROPERTY(qreal beta   READ beta   WRITE setBeta   NOTIFY betaChanged)
    Q_PROPERTY(qreal gamma  READ gamma  WRITE setGamma  NOTIFY gammaChanged)

public:
    explicit AddWeighted(QQuickItem *parent = 0);
    ~AddWeighted();

    void transform(cv::Mat& in, cv::Mat& out);

    QMat* input2();
    void setInput2(QMat* input2);

    qreal alpha() const;
    void setAlpha(qreal alpha);

    qreal beta() const;
    void setBeta(qreal beta);

    qreal gamma() const;
    void setGamma(qreal gamma);

signals:
    void input2Changed();
    void alphaChanged();
    void betaChanged();
    void gammaChanged();

private:
    QMat* m_input2;
    QMat* m_input2Internal;
    qreal m_alpha;
    qreal m_beta;
    qreal m_gamma;

};

inline QMat* AddWeighted::input2(){
    return m_input2;
}

inline void AddWeighted::setInput2(QMat* input2){
    m_input2 = input2;
    emit input2Changed();

    QMatFilter::transform();
}

inline qreal AddWeighted::alpha() const{
    return m_alpha;
}

inline void AddWeighted::setAlpha(qreal alpha){
    if ( m_alpha != alpha ){
        m_alpha = alpha;
        emit alphaChanged();

        QMatFilter::transform();
    }
}

inline qreal AddWeighted::beta() const{
    return m_beta;
}

inline void AddWeighted::setBeta(qreal beta){
    if ( m_beta != beta ){
        m_beta = beta;
        emit betaChanged();

        QMatFilter::transform();
    }
}

inline qreal AddWeighted::gamma() const{
    return m_gamma;
}

inline void AddWeighted::setGamma(qreal gamma){
    if ( gamma != m_gamma ){
        m_gamma = gamma;
        emit gammaChanged();

        QMatFilter::transform();
    }
}
```

```
// addweighted.cpp

#include "addweighted.h"

AddWeighted::AddWeighted(QQuickItem *parent)
    : QMatFilter(parent)
    , m_input2(new QMat)
    , m_input2Internal(m_input2)
{
}

AddWeighted::~AddWeighted(){
    delete m_input2Internal;
}

void AddWeighted::transform(cv::Mat& in, cv::Mat& out){
    cv::Mat* in2 = m_input2->cvMat();
    if ( in.size() == in2->size() && in.channels() == in2->channels() ){
        cv::addWeighted(in, m_alpha, *in2, m_beta, m_gamma, out);
    }
}
```

Two things to notice is that besides emitting the notifier when the value changes, we also call the
[QMatFilter::transform()]() function in each of the setters. After handling a few changes, the
transform() function in turn will call our implemented transform(cv::Mat& in, cv::Mat& out) function. The next thing to
notice is the m_input2Internal field, which is used to later delete the m_input2 matrix. To avoid checking for null
matrices all the time when handling value changes and transformations, we create a new matrix at m_input2 when our
object is initialized and keep the reference for further deletion. This avoids null pointers within the QML program as
well.

The final touch is to register the AddWeighted type to QML in TutorialPlugin::registerTypes :

```
#include "tutorial_plugin.h"
#include "countnonzeropixels.h"
#include "addweighted.h"

#include <qqml.h>

void TutorialPlugin::registerTypes(const char *uri)
{
    // @uri plugins.tutorial
    qmlRegisterType<CountNonZeroPixels>(uri, 1, 0, "CountNonZeroPixels");
    qmlRegisterType<AddWeighted>(       uri, 1, 0, "AddWeighted");
}
```

Compile the code, and place the generated dll/so file into Livekeys's plugins/tutorial directory. You can use the
following test program to test out the filter:

```
import QtQuick 2.3
import lcvcore 1.0 as Cv
import lcvimgproc 1.0
import tutorial 1.0

Grid{

    columns : 2

    property string imagePath : 'path/to/tutorial.jpg'

    Cv.ImRead{
        id : imgSource
        file : parent.imagePath
    }

    property Mat image2 : Cv.MatOp.create(imgSource.dimensions(), Cv.Mat.8U, 3, "#997822")

    AddWeighted{
        input : imgSource.output
        input2 : image2
        alpha : 0.6
        beta : 0.4
        gamma : 0
    }


}
```

The image below shows the result:

![](../src/images/api_filter.png)
