/****************************************************************************
 Copyright (c) 2008-2010 Ricardo Quesada
 Copyright (c) 2009      Valentin Milea
 Copyright (c) 2010-2012 cocos2d-x.org
 Copyright (c) 2011      Zynga Inc.
 Copyright (c) 2013-2014 Chukong Technologies Inc.

 http://www.cocos2d-x.org

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#ifndef __CCNODE_H__
#define __CCNODE_H__

#include "base/ccMacros.h"
#include "base/CCEventDispatcher.h"
#include "base/CCVector.h"
#include "math/CCAffineTransform.h"
#include "math/CCMath.h"
#include "renderer/ccGLStateCache.h"
#include "2d/CCScriptSupport.h"
#include "2d/CCProtocols.h"
#include "CCGL.h"

NS_CC_BEGIN

class GridBase;
class Touch;
class Action;
class LabelProtocol;
class Scheduler;
class ActionManager;
class Component;
class ComponentContainer;
class EventDispatcher;
class Scene;
class Renderer;
class GLProgram;
class GLProgramState;
#if CC_USE_PHYSICS
class PhysicsBody;
#endif

/**
 * @addtogroup base_nodes
 * @{
 */

enum {
    kNodeOnEnter,
    kNodeOnExit,
    kNodeOnEnterTransitionDidFinish,
    kNodeOnExitTransitionDidStart,
    kNodeOnCleanup
};

bool nodeComparisonLess(Node* n1, Node* n2);

class EventListener;

/** @brief Node是Scene Graph里最基本的元素，所有Scene Graph里的元素必须是Node对象或者它的子类。
常见的Node对象如下：Scene，Layer,Sprite,Menu,Label.

一个Node对象的主要特征如下：
-可以包含其他Node对象(通过`addChild`, `getChildByTag`, `removeChild`, 等等方法)
-可以计划周期性的回调(通过（`schedule`, `unschedule`, 等方法)
-可以执行动作(通过`runAction`, `stopAction`, 等方法)

继承Node生成它的一个子类意味着（满足一条或者多条）：
-重写init方法，初始化资源和周期性回调
-创建控制时间的回调
-重写`draw` 方法，渲染这个节点

Node的属性：
- position (默认: x=0, y=0)
 - scale (默认: x=1, y=1)
 - rotation (角度值描述, 顺时针方向) (默认: 0)
 - anchor point (默认: x=0, y=0)
 - contentSize (默认: width=0, height=0)
 - visible (默认: true)

 限制:
-一个Node是"空"对象，如果需要在屏幕上进行实际绘制，需要使用Sprite或者自定义Node的子类并重写`draw`方法。

 */

class CC_DLL Node : public Ref
{
public:
    /// 默认全部节点的tag
    static const int INVALID_TAG = -1;

    /// @{
    /// @name Constructor, Destructor and Initializers

    /**
     *分配空间并初始化一个node.
     * @return 一个标记“autorelease”的完成初始化工作的node
     */
    static Node * create(void);

    /**
     * 得到描述node的字符串，对debug工作有很大帮忙
     * @return A string
     * @js NA
     * @lua NA
     */
    virtual std::string getDescription() const;

    /// @} end of initializers



    /// @{
    /// @name Setters & Getters for Graphic Peroperties

    /**
     LocalZOrder 是父节点对子节点绘制进行排序的关键。
     一个Node对象的父亲根据LocalZOrder的值对全部孩子的绘制进行排序
     如果两个孩子拥有相同的LocalZOrder值，那么先加入孩子队列的先绘制。

     场景绘制节点的访问使用有序树的访问算法，参考（http://en.wikipedia.org/wiki/Tree_traversal#In-order）
     任何LocalZOder值小于0的Node将放在左子树上，其余将放在右子树上。
     
     @see `setGlobalZOrder`
     @see `setVertexZ`
     */
    virtual void setLocalZOrder(int localZOrder);

    CC_DEPRECATED_ATTRIBUTE virtual void setZOrder(int localZOrder) { setLocalZOrder(localZOrder); }
    /*
      `setLocalZOrder`的辅助函数，除非你很清楚自己做什么否则不要用.
     */
    virtual void _setLocalZOrder(int z);
    /**
     * 得到当前node的local Z order值
     *
     * @see `setLocalZOrder(int)`
     *
     * @return local Z order值.
     */
    virtual int getLocalZOrder() const { return _localZOrder; }
    CC_DEPRECATED_ATTRIBUTE virtual int getZOrder() const { return getLocalZOrder(); }

    /**
     Defines the oder in which the nodes are renderer.
     Nodes that have a Global Z Order lower, are renderer first.
     定义全部节点的绘制顺序。
     Global Z Order值越小的节点，越先绘制。
     
     In case two or more nodes have the same Global Z Order, the oder is not guaranteed.
     The only exception if the Nodes have a Global Z Order == 0. In that case, the Scene Graph order is used.
     如果两个节点有相同的Global Z Order值，则绘制的先后顺序不定。
     当节点的Global Z Order值为0时，绘制依照Scene Graph的顺序进行
     
     By default, all nodes have a Global Z Order = 0. That means that by default, the Scene Graph order is used to render the nodes.
     所有节点的Global Z Order默认值为0，在默认状态下，这些节点的绘制顺序依照Scene Graph的顺序进行
     
     Global Z Order is useful when you need to render nodes in an order different than the Scene Graph order.
     Global Z Order 在你不想按照Scene Graph的顺序进行绘制时是很有用的
     
     Limitations: Global Z Order can't be used used by Nodes that have SpriteBatchNode as one of their acenstors.
     And if ClippingNode is one of the ancestors, then "global Z order" will be relative to the ClippingNode.
     Global Z Order 不能被有SpriteBatchNode做为祖先之一的节点使用。如果ClippingNode是祖先之一，Global Z Order将与ClippingNode有关联。

     @see `setLocalZOrder()`
     @see `setVertexZ()`

     @since v3.0
     */
    virtual void setGlobalZOrder(float globalZOrder);
    /**
     * Returns the Node's Global Z Order.
     *
     * @see `setGlobalZOrder(int)`
     *
     * @return The node's global Z order
     */
    virtual float getGlobalZOrder() const { return _globalZOrder; }

    /**
     * Sets the scale (x) of the node      设置节点的scale (x) 值
     *
     * It is a scaling factor that multiplies the width of the node and its children.
     *用放缩因子乘以节点及其子节点的宽进行横向的放缩变化
     *
     * @param scaleX   The scale factor on X axis.
     */
    virtual void setScaleX(float scaleX);
    /**
     * Returns 返回节点x轴方向上的放缩因子
     *
     * @see setScaleX(float)
     *
     * @return The scale factor on X axis.
     */
    virtual float getScaleX() const;


    /**
     * Sets the scale (y) of the node. 设置节点的scale (y) 值
     *
     * It is a scaling factor that multiplies the height of the node and its children.
     * 用放缩因子乘以节点及其子节点的高进行纵向的放缩变化

     *
     * @param scaleY   The scale factor on Y axis. 节点y轴方向上的放缩因子
     */
    virtual void setScaleY(float scaleY);
    /**
     * Returns 返回节点y轴方向上的放缩因子
     *
     * @see `setScaleY(float)`
     *
     * @return The scale factor on Y axis.
     */
    virtual float getScaleY() const;

    /**
     * Changes the scale factor on Z axis of this node 设置节点z轴的放缩因子
     *
     * The Default value is 1.0 if you haven't changed it before. 默认值为1.0
     *
     * @param scaleY   The scale factor on Y axis. 
     */
    virtual void setScaleZ(float scaleZ);
    /**
     * Returns 返回节点z轴方向上的放缩因子
     *
     * @see `setScaleZ(float)`
     *
     * @return The scale factor on Z axis.
     */
    virtual float getScaleZ() const;


    /**
     * Sets the scale (x,y,z) of the node.
     *
     * It is a scaling factor that multiplies the width, height and depth of the node and its children.
     * 设置节点x，y，z轴方向的放缩因子，用放缩因子乘以节点及其子节点的长宽深进行放缩变化。
     *
     * @param scale     The scale factor for both X and Y axis.
     */
    virtual void setScale(float scale);
    /**
     * 得到x轴和y轴的等比例放缩因子
     *
     * @warning Assert when `_scaleX != _scaleY` 如果`_scaleX != _scaleY`会报错
     * @see setScale(float)
     *
     * @return The scale factor of the node.
     */
    virtual float getScale() const;

     /**
     * Sets the scale (x,y) of the node.
     *
     * It is a scaling factor that multiplies the width and height of the node and its children.
     * 分别设置x轴和y轴各自的放缩因子，用scaleX乘以宽得到节点及其孩子在x方向的放缩值,用scaleY乘以高得到节点及其孩子在y方向的放缩值。
     *
     * @param scaleX     The scale factor on X axis.
     * @param scaleY     The scale factor on Y axis.
     */
    virtual void setScale(float scaleX, float scaleY);

    /**
     * Sets the position (x,y) of the node in its parent's coordinate system.
     *传递Vec2参数，设置节点在父坐标系（OpenGL坐标系）的位置。
     * 
     * Usually we use `Vec2(x,y)` to compose Vec2 object.
     * This code snippet sets the node in the center of screen.
     * 我们通常用`Vec2(x,y)`来表示Vec2对象
     * 下面的代码段将节点设置在屏幕中央
     @code
     Size size = Director::getInstance()->getWinSize();
     node->setPosition( Vec2(size.width/2, size.height/2) )
     @endcode
     *
     * @param position  节点在OpenGL坐标系下的位置
     */
    virtual void setPosition(const Vec2 &position);
    /**
     * 得到节点在父坐标系（OpenGL坐标系）的位置。
     *
     * @see setPosition(const Vec2&)
     *
     * @return 节点在父坐标系（OpenGL坐标系）的位置，返回类型为Vec2
     * @code
     * In js and lua return value is table which contains x,y
     * 在js和lua中返回值为一个包含了x,y值的表
     * @endcode
     */
    virtual const Vec2& getPosition() const;
    /**
     * Sets the position (x,y) of the node in its parent's coordinate system.
     *参数x,y设置节点在父坐标系（OpenGL坐标系）的位置。
     * 
     * Passing two numbers (x,y) is much efficient than passing Vec2 object.
     * This method is bound to Lua and JavaScript.
     * Passing a number is 10 times faster than passing a object from Lua to c++
     *传递两个数字（x,y）比传递Vec2对象高效。
     * 这个方法有绑定到Lua和JavaScript.
     * 从Lua传递数字到c++中比直接传递对象要快10倍。
     * 
     @code
     // sample code in Lua
     local pos  = node::getPosition()  -- returns Vec2 object from C++
     node:setPosition(x, y)            -- pass x, y coordinate to C++
     @endcode
     *
     * @param x     X coordinate for position
     * @param y     Y coordinate for position
     */
    virtual void setPosition(float x, float y);
    /**
     * Gets position in a more efficient way, returns two number instead of a Vec2 object
     * 用更加高效的方式得到节点位置，返回两个数字而不是Vec2对象
     *
     * @see `setPosition(float, float)`
     * In js,out value not return
     */
    virtual void getPosition(float* x, float* y) const;
    /**
     * Gets/Sets x or y coordinate individually for position.
     * These methods are used in Lua and Javascript Bindings
     * 单独设置/获取x或y的坐标值
     * 这些方法都作用于Lua和Javascript 的绑定
     */
    virtual void  setPositionX(float x);
    virtual float getPositionX(void) const;
    virtual void  setPositionY(float y);
    virtual float getPositionY(void) const;

    /**
     * Sets the position (X, Y, and Z) in its parent's coordinate system
     * 传递Vec3参数，设置节点在父坐标系（OpenGL坐标系）的位置
     */
    virtual void setPosition3D(const Vec3& position);
    /**
     * 返回Vec3类型，得到节点在父坐标系（OpenGL坐标系）的位置
     */
    virtual Vec3 getPosition3D() const;

    /**
     * Sets the 'z' coordinate in the position. It is the OpenGL Z vertex value.
     * 设置'z'方向上的坐标值。
     *
     * The OpenGL depth buffer and depth testing are disabled by default. You need to turn them on
     * in order to use this property correctly.
     * OpenGL的深度缓冲和深度检测默认情况下是关闭的，你需要开启它们以便正常使用这个方法。
     * 
     *
     * `setPositionZ()` also sets the `setGlobalZValue()` with the positionZ as value.
     * `setPositionZ()` 方法同时也调用`setGlobalZValue()` 方法设置positionZ为GlobalZValue值。
     *
     * @see `setGlobalZValue()`
     *
     * @param vertexZ  这个节点的OpenGL z轴坐标值
     */
    virtual void setPositionZ(float positionZ);
    CC_DEPRECATED_ATTRIBUTE virtual void setVertexZ(float vertexZ) { setPositionZ(vertexZ); }

    /**
     * Gets position Z coordinate of this node.
     * 得到节点z轴的坐标值
     *
     * @see setPositionZ(float)
     *
     * @return 节点z轴的坐标值
     */
    virtual float getPositionZ() const;
    CC_DEPRECATED_ATTRIBUTE virtual float getVertexZ() const { return getPositionZ(); }

    /**
     * Changes the X skew angle of the node in degrees.
     *
     * The difference between `setRotationalSkew()` and `setSkew()` is that the first one simulate Flash's skew functionality
     * while the second one uses the real skew function.
     *
     * This angle describes the shear distortion in the X direction.
     * Thus, it is the angle between the Y coordinate and the left edge of the shape
     * The default skewX angle is 0. Positive values distort the node in a CW direction.
     *
     * @param skewX The X skew angle of the node in degrees.
     */
    virtual void setSkewX(float skewX);
    /**
     * Returns the X skew angle of the node in degrees.
     *
     * @see `setSkewX(float)`
     *
     * @return The X skew angle of the node in degrees.
     */
    virtual float getSkewX() const;


    /**
     * Changes the Y skew angle of the node in degrees.
     *
     * The difference between `setRotationalSkew()` and `setSkew()` is that the first one simulate Flash's skew functionality
     * while the second one uses the real skew function.
     *
     * This angle describes the shear distortion in the Y direction.
     * Thus, it is the angle between the X coordinate and the bottom edge of the shape
     * The default skewY angle is 0. Positive values distort the node in a CCW direction.
     *
     * @param skewY    The Y skew angle of the node in degrees.
     */
    virtual void setSkewY(float skewY);
    /**
     * Returns the Y skew angle of the node in degrees.
     *
     * @see `setSkewY(float)`
     *
     * @return The Y skew angle of the node in degrees.
     */
    virtual float getSkewY() const;


    /**
     * Sets the anchor point in percent.
     *
     * anchorPoint is the point around which all transformations and positioning manipulations take place.
     * It's like a pin in the node where it is "attached" to its parent.
     * The anchorPoint is normalized, like a percentage. (0,0) means the bottom-left corner and (1,1) means the top-right corner.
     * But you can use values higher than (1,1) and lower than (0,0) too.
     * The default anchorPoint is (0.5,0.5), so it starts in the center of the node.
     * @note If node has a physics body, the anchor must be in the middle, you cann't change this to other value.
     *
     * @param anchorPoint   The anchor point of node.
     */
    virtual void setAnchorPoint(const Vec2& anchorPoint);
    /**
     * Returns the anchor point in percent.
     *
     * @see `setAnchorPoint(const Vec2&)`
     *
     * @return The anchor point of node.
     */
    virtual const Vec2& getAnchorPoint() const;
    /**
     * Returns the anchorPoint in absolute pixels.
     *
     * @warning You can only read it. If you wish to modify it, use anchorPoint instead.
     * @see `getAnchorPoint()`
     *
     * @return The anchor point in absolute pixels.
     */
    virtual const Vec2& getAnchorPointInPoints() const;


    /**
     * Sets the untransformed size of the node.
     *
     * The contentSize remains the same no matter the node is scaled or rotated.
     * All nodes has a size. Layer and Scene has the same size of the screen.
     *
     * @param contentSize   The untransformed size of the node.
     */
    virtual void setContentSize(const Size& contentSize);
    /**
     * Returns the untransformed size of the node.
     *
     * @see `setContentSize(const Size&)`
     *
     * @return The untransformed size of the node.
     */
    virtual const Size& getContentSize() const;


    /**
     * Sets whether the node is visible
     *
     * The default value is true, a node is default to visible
     *
     * @param visible   true if the node is visible, false if the node is hidden.
     */
    virtual void setVisible(bool visible);
    /**
     * Determines if the node is visible
     *
     * @see `setVisible(bool)`
     *
     * @return true if the node is visible, false if the node is hidden.
     */
    virtual bool isVisible() const;


    /**
     * Sets the rotation (angle) of the node in degrees.
     *
     * 0 is the default rotation angle.
     * Positive values rotate node clockwise, and negative values for anti-clockwise.
     *
     * @param rotation     The rotation of the node in degrees.
     */
    virtual void setRotation(float rotation);
    /**
     * Returns the rotation of the node in degrees.
     *
     * @see `setRotation(float)`
     *
     * @return The rotation of the node in degrees.
     */
    virtual float getRotation() const;

    /**
     * Sets the rotation (X,Y,Z) in degrees.
     * Useful for 3d rotations
     */
    virtual void setRotation3D(const Vec3& rotation);
    /**
     * returns the rotation (X,Y,Z) in degrees.
     */
    virtual Vec3 getRotation3D() const;

    /**
     * Sets the X rotation (angle) of the node in degrees which performs a horizontal rotational skew.
     *
     * The difference between `setRotationalSkew()` and `setSkew()` is that the first one simulate Flash's skew functionality
     * while the second one uses the real skew function.
     *
     * 0 is the default rotation angle.
     * Positive values rotate node clockwise, and negative values for anti-clockwise.
     *
     * @param rotationX    The X rotation in degrees which performs a horizontal rotational skew.
     */
    virtual void setRotationSkewX(float rotationX);
    CC_DEPRECATED_ATTRIBUTE virtual void setRotationX(float rotationX) { return setRotationSkewX(rotationX); }

    /**
     * Gets the X rotation (angle) of the node in degrees which performs a horizontal rotation skew.
     *
     * @see `setRotationSkewX(float)`
     *
     * @return The X rotation in degrees.
     */
    virtual float getRotationSkewX() const;
    CC_DEPRECATED_ATTRIBUTE virtual float getRotationX() const { return getRotationSkewX(); }

    /**
     * Sets the Y rotation (angle) of the node in degrees which performs a vertical rotational skew.
     *
     * The difference between `setRotationalSkew()` and `setSkew()` is that the first one simulate Flash's skew functionality
     * while the second one uses the real skew function.
     *
     * 0 is the default rotation angle.
     * Positive values rotate node clockwise, and negative values for anti-clockwise.
     *
     * @param rotationY    The Y rotation in degrees.
     */
    virtual void setRotationSkewY(float rotationY);
    CC_DEPRECATED_ATTRIBUTE virtual void setRotationY(float rotationY) { return setRotationSkewY(rotationY); }

    /**
     * Gets the Y rotation (angle) of the node in degrees which performs a vertical rotational skew.
     *
     * @see `setRotationSkewY(float)`
     *
     * @return The Y rotation in degrees.
     */
    virtual float getRotationSkewY() const;
    CC_DEPRECATED_ATTRIBUTE virtual float getRotationY() const { return getRotationSkewY(); }

    /**
     * Sets the arrival order when this node has a same ZOrder with other children.
     *
     * A node which called addChild subsequently will take a larger arrival order,
     * If two children have the same Z order, the child with larger arrival order will be drawn later.
     *
     * @warning This method is used internally for localZOrder sorting, don't change this manually
     *
     * @param orderOfArrival   The arrival order.
     */
    void setOrderOfArrival(int orderOfArrival);
    /**
     * Returns the arrival order, indicates which children is added previously.
     *
     * @see `setOrderOfArrival(unsigned int)`
     *
     * @return The arrival order.
     */
    int getOrderOfArrival() const;


    /** @deprecated No longer needed
    * @js NA
    * @lua NA
    */
    CC_DEPRECATED_ATTRIBUTE void setGLServerState(int serverState) { /* ignore */ };
    /** @deprecated No longer needed
    * @js NA
    * @lua NA
    */
    CC_DEPRECATED_ATTRIBUTE int getGLServerState() const { return 0; }

    /**
     * Sets whether the anchor point will be (0,0) when you position this node.
     *
     * This is an internal method, only used by Layer and Scene. Don't call it outside framework.
     * The default value is false, while in Layer and Scene are true
     *
     * @param ignore    true if anchor point will be (0,0) when you position this node
     * @todo This method should be renamed as setIgnoreAnchorPointForPosition(bool) or something with "set"
     */
    virtual void ignoreAnchorPointForPosition(bool ignore);
    /**
     * Gets whether the anchor point will be (0,0) when you position this node.
     *
     * @see `ignoreAnchorPointForPosition(bool)`
     *
     * @return true if the anchor point will be (0,0) when you position this node.
     */
    virtual bool isIgnoreAnchorPointForPosition() const;

    /// @}  end of Setters & Getters for Graphic Properties


    /// @{
    /// @name Children and Parent

    /**
     * Adds a child to the container with z-order as 0.
     *
     * If the child is added to a 'running' node, then 'onEnter' and 'onEnterTransitionDidFinish' will be called immediately.
     *
     * @param child A child node
     */
    virtual void addChild(Node * child);
    /**
     * Adds a child to the container with a local z-order
     *
     * If the child is added to a 'running' node, then 'onEnter' and 'onEnterTransitionDidFinish' will be called immediately.
     *
     * @param child     A child node
     * @param zOrder    Z order for drawing priority. Please refer to `setLocalZOrder(int)`
     */
    virtual void addChild(Node * child, int localZOrder);
    /**
     * Adds a child to the container with z order and tag
     *
     * If the child is added to a 'running' node, then 'onEnter' and 'onEnterTransitionDidFinish' will be called immediately.
     *
     * @param child     A child node
     * @param zOrder    Z order for drawing priority. Please refer to `setLocalZOrder(int)`
     * @param tag       An integer to identify the node easily. Please refer to `setTag(int)`
     */
    virtual void addChild(Node* child, int localZOrder, int tag);
    /**
     * Gets a child from the container with its tag
     *
     * @param tag   An identifier to find the child node.
     *
     * @return a Node object whose tag equals to the input parameter
     */
    virtual Node * getChildByTag(int tag);
    /**
     * Returns the array of the node's children
     *
     * @return the array the node's children
     */
    virtual Vector<Node*>& getChildren() { return _children; }
    virtual const Vector<Node*>& getChildren() const { return _children; }
    
    /** 
     * Returns the amount of children
     *
     * @return The amount of children.
     */
    virtual ssize_t getChildrenCount() const;

    /**
     * Sets the parent node
     *
     * @param parent    A pointer to the parent node
     */
    virtual void setParent(Node* parent);
    /**
     * Returns a pointer to the parent node
     *
     * @see `setParent(Node*)`
     *
     * @returns A pointer to the parent node
     */
    virtual Node* getParent() { return _parent; }
    virtual const Node* getParent() const { return _parent; }


    ////// REMOVES //////

    /**
     * Removes this node itself from its parent node with a cleanup.
     * If the node orphan, then nothing happens.
     * @see `removeFromParentAndCleanup(bool)`
     */
    virtual void removeFromParent();
    /**
     * Removes this node itself from its parent node.
     * If the node orphan, then nothing happens.
     * @param cleanup   true if all actions and callbacks on this node should be removed, false otherwise.
     * @js removeFromParent
     * @lua removeFromParent
     */
    virtual void removeFromParentAndCleanup(bool cleanup);

    /**
     * Removes a child from the container. It will also cleanup all running actions depending on the cleanup parameter.
     *
     * @param child     The child node which will be removed.
     * @param cleanup   true if all running actions and callbacks on the child node will be cleanup, false otherwise.
     */
    virtual void removeChild(Node* child, bool cleanup = true);

    /**
     * Removes a child from the container by tag value. It will also cleanup all running actions depending on the cleanup parameter
     *
     * @param tag       An interger number that identifies a child node
     * @param cleanup   true if all running actions and callbacks on the child node will be cleanup, false otherwise.
     */
    virtual void removeChildByTag(int tag, bool cleanup = true);
    /**
     * Removes all children from the container with a cleanup.
     *
     * @see `removeAllChildrenWithCleanup(bool)`
     */
    virtual void removeAllChildren();
    /**
     * Removes all children from the container, and do a cleanup to all running actions depending on the cleanup parameter.
     *
     * @param cleanup   true if all running actions on all children nodes should be cleanup, false oterwise.
     * @js removeAllChildren
     * @lua removeAllChildren
     */
    virtual void removeAllChildrenWithCleanup(bool cleanup);

    /**
     * Reorders a child according to a new z value.
     *
     * @param child     An already added child node. It MUST be already added.
     * @param localZOrder Z order for drawing priority. Please refer to setLocalZOrder(int)
     */
    virtual void reorderChild(Node * child, int localZOrder);

    /**
     * Sorts the children array once before drawing, instead of every time when a child is added or reordered.
     * This appraoch can improves the performance massively.
     * @note Don't call this manually unless a child added needs to be removed in the same frame
     */
    virtual void sortAllChildren();

    /// @} end of Children and Parent
    
    /// @{
    /// @name Tag & User data

    /**
     * Returns a tag that is used to identify the node easily.
     *
     * @return An integer that identifies the node.
     */
    virtual int getTag() const;
    /**
     * Changes the tag that is used to identify the node easily.
     *
     * Please refer to getTag for the sample code.
     *
     * @param tag   A integer that identifies the node.
     */
    virtual void setTag(int tag);

    
    /**
     * Returns a custom user data pointer
     *
     * You can set everything in UserData pointer, a data block, a structure or an object.
     *
     * @return A custom user data pointer
     * @js NA
     * @lua NA
     */
    virtual void* getUserData() { return _userData; }
    /**
    * @js NA
    * @lua NA
    */
    virtual const void* getUserData() const { return _userData; }

    /**
     * Sets a custom user data pointer
     *
     * You can set everything in UserData pointer, a data block, a structure or an object, etc.
     * @warning Don't forget to release the memory manually,
     *          especially before you change this data pointer, and before this node is autoreleased.
     *
     * @param userData  A custom user data pointer
     * @js NA
     * @lua NA
     */
    virtual void setUserData(void *userData);

    /**
     * Returns a user assigned Object
     *
     * Similar to userData, but instead of holding a void* it holds an object
     *
     * @return A user assigned Object
     * @js NA
     * @lua NA
     */
    virtual Ref* getUserObject() { return _userObject; }
    /**
    * @js NA
    * @lua NA
    */
    virtual const Ref* getUserObject() const { return _userObject; }

    /**
     * Returns a user assigned Object
     *
     * Similar to UserData, but instead of holding a void* it holds an object.
     * The UserObject will be retained once in this method,
     * and the previous UserObject (if existed) will be released.
     * The UserObject will be released in Node's destructor.
     *
     * @param userObject    A user assigned Object
     */
    virtual void setUserObject(Ref *userObject);

    /// @} end of Tag & User Data


    /// @{
    /// @name GLProgram
    /**
     * Return the GLProgram (shader) currently used for this node
     *
     * @return The GLProgram (shader) currently used for this node
     */
    GLProgram* getGLProgram();
    CC_DEPRECATED_ATTRIBUTE GLProgram* getShaderProgram() { return getGLProgram(); }

    GLProgramState *getGLProgramState();
    void setGLProgramState(GLProgramState *glProgramState);

    /**
     * Sets the shader program for this node
     *
     * Since v2.0, each rendering node must set its shader program.
     * It should be set in initialize phase.
     @code
     node->setGLrProgram(GLProgramCache::getInstance()->getProgram(GLProgram::SHADER_NAME_POSITION_TEXTURE_COLOR));
     @endcode
     *
     * @param shaderProgram The shader program
     */
    void setGLProgram(GLProgram *glprogram);
    CC_DEPRECATED_ATTRIBUTE void setShaderProgram(GLProgram *glprogram) { setGLProgram(glprogram); }
    /// @} end of Shader Program


    /**
     * Returns whether or not the node is "running".
     *
     * If the node is running it will accept event callbacks like onEnter(), onExit(), update()
     *
     * @return Whether or not the node is running.
     */
    virtual bool isRunning() const;

    /**
     * Schedules for lua script.
     * @js NA
     */
    void scheduleUpdateWithPriorityLua(int handler, int priority);

    /// @}  end Script Bindings


    /// @{
    /// @name Event Callbacks

    /**
     * Event callback that is invoked every time when Node enters the 'stage'.
     * If the Node enters the 'stage' with a transition, this event is called when the transition starts.
     * During onEnter you can't access a "sister/brother" node.
     * If you override onEnter, you shall call its parent's one, e.g., Node::onEnter().
     * @js NA
     * @lua NA
     */
    virtual void onEnter();

    /** Event callback that is invoked when the Node enters in the 'stage'.
     * If the Node enters the 'stage' with a transition, this event is called when the transition finishes.
     * If you override onEnterTransitionDidFinish, you shall call its parent's one, e.g. Node::onEnterTransitionDidFinish()
     * @js NA
     * @lua NA
     */
    virtual void onEnterTransitionDidFinish();

    /**
     * Event callback that is invoked every time the Node leaves the 'stage'.
     * If the Node leaves the 'stage' with a transition, this event is called when the transition finishes.
     * During onExit you can't access a sibling node.
     * If you override onExit, you shall call its parent's one, e.g., Node::onExit().
     * @js NA
     * @lua NA
     */
    virtual void onExit();

    /**
     * Event callback that is called every time the Node leaves the 'stage'.
     * If the Node leaves the 'stage' with a transition, this callback is called when the transition starts.
     * @js NA
     * @lua NA
     */
    virtual void onExitTransitionDidStart();

    /// @} end of event callbacks.


    /**
     * Stops all running actions and schedulers
     */
    virtual void cleanup();

    /**
     * Override this method to draw your own node.
     * The following GL states will be enabled by default:
     * - `glEnableClientState(GL_VERTEX_ARRAY);`
     * - `glEnableClientState(GL_COLOR_ARRAY);`
     * - `glEnableClientState(GL_TEXTURE_COORD_ARRAY);`
     * - `glEnable(GL_TEXTURE_2D);`
     * AND YOU SHOULD NOT DISABLE THEM AFTER DRAWING YOUR NODE
     * But if you enable any other GL state, you should disable it after drawing your node.
     */
    virtual void draw(Renderer *renderer, const Mat4& transform, bool transformUpdated);
    virtual void draw() final;

    /**
     * Visits this node's children and draw them recursively.
     */
    virtual void visit(Renderer *renderer, const Mat4& parentTransform, bool parentTransformUpdated);
    virtual void visit() final;


    /** Returns the Scene that contains the Node.
     It returns `nullptr` if the node doesn't belong to any Scene.
     This function recursively calls parent->getScene() until parent is a Scene object. The results are not cached. It is that the user caches the results in case this functions is being used inside a loop.
     */
    virtual Scene* getScene();

    /**
     * Returns an AABB (axis-aligned bounding-box) in its parent's coordinate system.
     *
     * @return An AABB (axis-aligned bounding-box) in its parent's coordinate system
     */
    virtual Rect getBoundingBox() const;

    /** @deprecated Use getBoundingBox instead */
    CC_DEPRECATED_ATTRIBUTE inline virtual Rect boundingBox() const { return getBoundingBox(); }

    virtual void setEventDispatcher(EventDispatcher* dispatcher);
    virtual EventDispatcher* getEventDispatcher() const { return _eventDispatcher; };

    /// @{
    /// @name Actions

    /**
     * Sets the ActionManager object that is used by all actions.
     *
     * @warning If you set a new ActionManager, then previously created actions will be removed.
     *
     * @param actionManager     A ActionManager object that is used by all actions.
     */
    virtual void setActionManager(ActionManager* actionManager);
    /**
     * Gets the ActionManager object that is used by all actions.
     * @see setActionManager(ActionManager*)
     * @return A ActionManager object.
     */
    virtual ActionManager* getActionManager() { return _actionManager; }
    virtual const ActionManager* getActionManager() const { return _actionManager; }

    /**
     * Executes an action, and returns the action that is executed.
     *
     * This node becomes the action's target. Refer to Action::getTarget()
     * @warning Actions don't retain their target.
     *
     * @return An Action pointer
     */
    Action* runAction(Action* action);

    /**
     * Stops and removes all actions from the running action list .
     */
    void stopAllActions();

    /**
     * Stops and removes an action from the running action list.
     *
     * @param action    The action object to be removed.
     */
    void stopAction(Action* action);

    /**
     * Removes an action from the running action list by its tag.
     *
     * @param tag   A tag that indicates the action to be removed.
     */
    void stopActionByTag(int tag);

    /**
     * Gets an action from the running action list by its tag.
     *
     * @see `setTag(int)`, `getTag()`.
     *
     * @return The action object with the given tag.
     */
    Action* getActionByTag(int tag);

    /**
     * Returns the numbers of actions that are running plus the ones that are schedule to run (actions in actionsToAdd and actions arrays).
     *
     * Composable actions are counted as 1 action. Example:
     *    If you are running 1 Sequence of 7 actions, it will return 1.
     *    If you are running 7 Sequences of 2 actions, it will return 7.
     * @todo Rename to getNumberOfRunningActions()
     *
     * @return The number of actions that are running plus the ones that are schedule to run
     */
    ssize_t getNumberOfRunningActions() const;

    /** @deprecated Use getNumberOfRunningActions() instead */
    CC_DEPRECATED_ATTRIBUTE ssize_t numberOfRunningActions() const { return getNumberOfRunningActions(); };

    /// @} end of Actions


    /// @{
    /// @name Scheduler and Timer

    /**
     * Sets a Scheduler object that is used to schedule all "updates" and timers.
     *
     * @warning If you set a new Scheduler, then previously created timers/update are going to be removed.
     * @param scheduler     A Shdeduler object that is used to schedule all "update" and timers.
     */
    virtual void setScheduler(Scheduler* scheduler);
    /**
     * Gets a Sheduler object.
     *
     * @see setScheduler(Scheduler*)
     * @return A Scheduler object.
     */
    virtual Scheduler* getScheduler() { return _scheduler; }
    virtual const Scheduler* getScheduler() const { return _scheduler; }


    /**
     * Checks whether a selector is scheduled.
     *
     * @param selector      A function selector
     * @return Whether the funcion selector is scheduled.
     * @js NA
     * @lua NA
     */
    bool isScheduled(SEL_SCHEDULE selector);

    /**
     * Schedules the "update" method.
     *
     * It will use the order number 0. This method will be called every frame.
     * Scheduled methods with a lower order value will be called before the ones that have a higher order value.
     * Only one "update" method could be scheduled per node.
     * @js NA
     * @lua NA
     */
    void scheduleUpdate(void);

    /**
     * Schedules the "update" method with a custom priority.
     *
     * This selector will be called every frame.
     * Scheduled methods with a lower priority will be called before the ones that have a higher value.
     * Only one "update" selector could be scheduled per node (You can't have 2 'update' selectors).
     * @js NA
     * @lua NA
     */
    void scheduleUpdateWithPriority(int priority);

    /*
     * Unschedules the "update" method.
     * @see scheduleUpdate();
     */
    void unscheduleUpdate(void);

    /**
     * Schedules a custom selector.
     *
     * If the selector is already scheduled, then the interval parameter will be updated without scheduling it again.
     @code
     // firstly, implement a schedule function
     void MyNode::TickMe(float dt);
     // wrap this function into a selector via schedule_selector marco.
     this->schedule(schedule_selector(MyNode::TickMe), 0, 0, 0);
     @endcode
     *
     * @param selector  The SEL_SCHEDULE selector to be scheduled.
     * @param interval  Tick interval in seconds. 0 means tick every frame. If interval = 0, it's recommended to use scheduleUpdate() instead.
     * @param repeat    The selector will be excuted (repeat + 1) times, you can use kRepeatForever for tick infinitely.
     * @param delay     The amount of time that the first tick will wait before execution.
     * @lua NA
     */
    void schedule(SEL_SCHEDULE selector, float interval, unsigned int repeat, float delay);

    /**
     * Schedules a custom selector with an interval time in seconds.
     * @see `schedule(SEL_SCHEDULE, float, unsigned int, float)`
     *
     * @param selector      The SEL_SCHEDULE selector to be scheduled.
     * @param interval      Callback interval time in seconds. 0 means tick every frame,
     * @lua NA
     */
    void schedule(SEL_SCHEDULE selector, float interval);

    /**
     * Schedules a selector that runs only once, with a delay of 0 or larger
     * @see `schedule(SEL_SCHEDULE, float, unsigned int, float)`
     *
     * @param selector      The SEL_SCHEDULE selector to be scheduled.
     * @param delay         The amount of time that the first tick will wait before execution.
     * @lua NA
     */
    void scheduleOnce(SEL_SCHEDULE selector, float delay);

    /**
     * Schedules a custom selector, the scheduled selector will be ticked every frame
     * @see schedule(SEL_SCHEDULE, float, unsigned int, float)
     *
     * @param selector      A function wrapped as a selector
     * @lua NA
     */
    void schedule(SEL_SCHEDULE selector);

    /**
     * Unschedules a custom selector.
     * @see `schedule(SEL_SCHEDULE, float, unsigned int, float)`
     *
     * @param selector      A function wrapped as a selector
     * @lua NA
     */
    void unschedule(SEL_SCHEDULE selector);

    /**
     * Unschedule all scheduled selectors: custom selectors, and the 'update' selector.
     * Actions are not affected by this method.
     * @lua NA
     */
    void unscheduleAllSelectors(void);

    /**
     * Resumes all scheduled selectors, actions and event listeners.
     * This method is called internally by onEnter
     */
    void resume(void);
    /**
     * Pauses all scheduled selectors, actions and event listeners..
     * This method is called internally by onExit
     */
    void pause(void);

    /**
     * Resumes all scheduled selectors, actions and event listeners.
     * This method is called internally by onEnter
     */
    CC_DEPRECATED_ATTRIBUTE void resumeSchedulerAndActions(void);
    /**
     * Pauses all scheduled selectors, actions and event listeners..
     * This method is called internally by onExit
     */
    CC_DEPRECATED_ATTRIBUTE void pauseSchedulerAndActions(void);

    /*
     * Update method will be called automatically every frame if "scheduleUpdate" is called, and the node is "live"
     */
    virtual void update(float delta);

    /// @} end of Scheduler and Timer

    /// @{
    /// @name Transformations

    /**
     * Calls children's updateTransform() method recursively.
     *
     * This method is moved from Sprite, so it's no longer specific to Sprite.
     * As the result, you apply SpriteBatchNode's optimization on your customed Node.
     * e.g., `batchNode->addChild(myCustomNode)`, while you can only addChild(sprite) before.
     */
    virtual void updateTransform();

    /**
     * Returns the matrix that transform the node's (local) space coordinates into the parent's space coordinates.
     * The matrix is in Pixels.
     */
    virtual const Mat4& getNodeToParentTransform() const;
    virtual AffineTransform getNodeToParentAffineTransform() const;

    /** 
     * Sets the Transformation matrix manually.
     */
    virtual void setNodeToParentTransform(const Mat4& transform);

    /** @deprecated use getNodeToParentTransform() instead */
    CC_DEPRECATED_ATTRIBUTE inline virtual AffineTransform nodeToParentTransform() const { return getNodeToParentAffineTransform(); }

    /**
     * Returns the matrix that transform parent's space coordinates to the node's (local) space coordinates.
     * The matrix is in Pixels.
     */
    virtual const Mat4& getParentToNodeTransform() const;
    virtual AffineTransform getParentToNodeAffineTransform() const;

    /** @deprecated Use getParentToNodeTransform() instead */
    CC_DEPRECATED_ATTRIBUTE inline virtual AffineTransform parentToNodeTransform() const { return getParentToNodeAffineTransform(); }

    /**
     * Returns the world affine transform matrix. The matrix is in Pixels.
     */
    virtual Mat4 getNodeToWorldTransform() const;
    virtual AffineTransform getNodeToWorldAffineTransform() const;

    /** @deprecated Use getNodeToWorldTransform() instead */
    CC_DEPRECATED_ATTRIBUTE inline virtual AffineTransform nodeToWorldTransform() const { return getNodeToWorldAffineTransform(); }

    /**
     * Returns the inverse world affine transform matrix. The matrix is in Pixels.
     */
    virtual Mat4 getWorldToNodeTransform() const;
    virtual AffineTransform getWorldToNodeAffineTransform() const;


    /** @deprecated Use getWorldToNodeTransform() instead */
    CC_DEPRECATED_ATTRIBUTE inline virtual AffineTransform worldToNodeTransform() const { return getWorldToNodeAffineTransform(); }

    /// @} end of Transformations


    /// @{
    /// @name Coordinate Converters

    /**
     * Converts a Vec2 to node (local) space coordinates. The result is in Points.
     */
    Vec2 convertToNodeSpace(const Vec2& worldPoint) const;

    /**
     * Converts a Vec2 to world space coordinates. The result is in Points.
     */
    Vec2 convertToWorldSpace(const Vec2& nodePoint) const;

    /**
     * Converts a Vec2 to node (local) space coordinates. The result is in Points.
     * treating the returned/received node point as anchor relative.
     */
    Vec2 convertToNodeSpaceAR(const Vec2& worldPoint) const;

    /**
     * Converts a local Vec2 to world space coordinates.The result is in Points.
     * treating the returned/received node point as anchor relative.
     */
    Vec2 convertToWorldSpaceAR(const Vec2& nodePoint) const;

    /**
     * convenience methods which take a Touch instead of Vec2
     */
    Vec2 convertTouchToNodeSpace(Touch * touch) const;

    /**
     * converts a Touch (world coordinates) into a local coordinate. This method is AR (Anchor Relative).
     */
    Vec2 convertTouchToNodeSpaceAR(Touch * touch) const;

	/**
     *  Sets an additional transform matrix to the node.
     *
     *  In order to remove it, call it again with the argument `nullptr`
     *
     *  @note The additional transform will be concatenated at the end of getNodeToParentTransform.
     *        It could be used to simulate `parent-child` relationship between two nodes (e.g. one is in BatchNode, another isn't).
     */
    void setAdditionalTransform(Mat4* additionalTransform);
    void setAdditionalTransform(const AffineTransform& additionalTransform);

    /// @} end of Coordinate Converters

      /// @{
    /// @name component functions
    /**
     *   gets a component by its name
     */
    Component* getComponent(const std::string& pName);

    /**
     *   adds a component
     */
    virtual bool addComponent(Component *pComponent);

    /**
     *   removes a component by its name
     */
    virtual bool removeComponent(const std::string& pName);

    /**
     *   removes all components
     */
    virtual void removeAllComponents();
    /// @} end of component functions


#if CC_USE_PHYSICS
    /**
     *   set the PhysicsBody that let the sprite effect with physics
     * @note This method will set anchor point to Vec2::ANCHOR_MIDDLE if body not null, and you cann't change anchor point if node has a physics body.
     */
    void setPhysicsBody(PhysicsBody* body);

    /**
     *   get the PhysicsBody the sprite have
     */
    PhysicsBody* getPhysicsBody() const;

#endif
    
    // overrides
    virtual GLubyte getOpacity() const;
    virtual GLubyte getDisplayedOpacity() const;
    virtual void setOpacity(GLubyte opacity);
    virtual void updateDisplayedOpacity(GLubyte parentOpacity);
    virtual bool isCascadeOpacityEnabled() const;
    virtual void setCascadeOpacityEnabled(bool cascadeOpacityEnabled);
    
    virtual const Color3B& getColor(void) const;
    virtual const Color3B& getDisplayedColor() const;
    virtual void setColor(const Color3B& color);
    virtual void updateDisplayedColor(const Color3B& parentColor);
    virtual bool isCascadeColorEnabled() const;
    virtual void setCascadeColorEnabled(bool cascadeColorEnabled);
    
    virtual void setOpacityModifyRGB(bool bValue) {CC_UNUSED_PARAM(bValue);}
    virtual bool isOpacityModifyRGB() const { return false; };
    
CC_CONSTRUCTOR_ACCESS:
    // Nodes should be created using create();
    Node();
    virtual ~Node();

    virtual bool init();

protected:
    /// lazy allocs
    void childrenAlloc(void);
    
    /// helper that reorder a child
    void insertChild(Node* child, int z);

    /// Removes a child, call child->onExit(), do cleanup, remove it from children array.
    void detachChild(Node *child, ssize_t index, bool doCleanup);

    /// Convert cocos2d coordinates to UI windows coordinate.
    Vec2 convertToWindowSpace(const Vec2& nodePoint) const;

    Mat4 transform(const Mat4 &parentTransform);

    virtual void updateCascadeOpacity();
    virtual void disableCascadeOpacity();
    virtual void updateCascadeColor();
    virtual void disableCascadeColor();
    virtual void updateColor() {}
    
#if CC_USE_PHYSICS
    virtual void updatePhysicsBodyPosition(Scene* layer);
    virtual void updatePhysicsBodyRotation(Scene* layer);
#endif // CC_USE_PHYSICS

    float _rotationX;               ///< rotation on the X-axis
    float _rotationY;               ///< rotation on the Y-axis

    // rotation Z is decomposed in 2 to simulate Skew for Flash animations
    float _rotationZ_X;             ///< rotation angle on Z-axis, component X
    float _rotationZ_Y;             ///< rotation angle on Z-axis, component Y

    float _scaleX;                  ///< scaling factor on x-axis
    float _scaleY;                  ///< scaling factor on y-axis
    float _scaleZ;                  ///< scaling factor on z-axis

    Vec2 _position;                ///< position of the node
    float _positionZ;               ///< OpenGL real Z position

    float _skewX;                   ///< skew angle on x-axis
    float _skewY;                   ///< skew angle on y-axis

    Vec2 _anchorPointInPoints;     ///< anchor point in points
    Vec2 _anchorPoint;             ///< anchor point normalized (NOT in points)

    Size _contentSize;              ///< untransformed size of the node

    Mat4 _modelViewTransform;    ///< ModelView transform of the Node.

    // "cache" variables are allowed to be mutable
    mutable Mat4 _transform;      ///< transform
    mutable bool _transformDirty;   ///< transform dirty flag
    mutable Mat4 _inverse;        ///< inverse transform
    mutable bool _inverseDirty;     ///< inverse transform dirty flag
    mutable Mat4 _additionalTransform; ///< transform
    bool _useAdditionalTransform;   ///< The flag to check whether the additional transform is dirty
    bool _transformUpdated;         ///< Whether or not the Transform object was updated since the last frame

    int _localZOrder;               ///< Local order (relative to its siblings) used to sort the node
    float _globalZOrder;            ///< Global order used to sort the node

    Vector<Node*> _children;        ///< array of children nodes
    Node *_parent;                  ///< weak reference to parent node

    int _tag;                         ///< a tag. Can be any number you assigned just to identify this node
    
    std::string _name;               ///<a string label, an user defined string to identify this node

    void *_userData;                ///< A user assingned void pointer, Can be point to any cpp object
    Ref *_userObject;               ///< A user assigned Object

    GLProgramState *_glProgramState; ///< OpenGL Program State

    int _orderOfArrival;            ///< used to preserve sequence while sorting children with the same localZOrder

    Scheduler *_scheduler;          ///< scheduler used to schedule timers and updates

    ActionManager *_actionManager;  ///< a pointer to ActionManager singleton, which is used to handle all the actions

    EventDispatcher* _eventDispatcher;  ///< event dispatcher used to dispatch all kinds of events

    bool _running;                  ///< is running

    bool _visible;                  ///< is this node visible

    bool _ignoreAnchorPointForPosition; ///< true if the Anchor Vec2 will be (0,0) when you position the Node, false otherwise.
                                          ///< Used by Layer and Scene.

    bool _reorderChildDirty;          ///< children order dirty flag
    bool _isTransitionFinished;       ///< flag to indicate whether the transition was finished

#if CC_ENABLE_SCRIPT_BINDING
    int _scriptHandler;               ///< script handler for onEnter() & onExit(), used in Javascript binding and Lua binding.
    int _updateScriptHandler;         ///< script handler for update() callback per frame, which is invoked from lua & javascript.
    ccScriptType _scriptType;         ///< type of script binding, lua or javascript
#endif
    
    ComponentContainer *_componentContainer;        ///< Dictionary of components

#if CC_USE_PHYSICS
    PhysicsBody* _physicsBody;        ///< the physicsBody the node have
#endif
    
    // opacity controls
    GLubyte		_displayedOpacity;
    GLubyte     _realOpacity;
    Color3B	    _displayedColor;
    Color3B     _realColor;
    bool		_cascadeColorEnabled;
    bool        _cascadeOpacityEnabled;

    static int s_globalOrderOfArrival;
    
private:
    CC_DISALLOW_COPY_AND_ASSIGN(Node);
    
#if CC_USE_PHYSICS
    friend class Layer;
#endif //CC_USTPS
};

// NodeRGBA

/** NodeRGBA is a subclass of Node that implements the RGBAProtocol protocol.
 
 All features from Node are valid, plus the following new features:
 - opacity
 - RGB colors
 
 Opacity/Color propagates into children that conform to the RGBAProtocol if cascadeOpacity/cascadeColor is enabled.
 @since v2.1
 */
class CC_DLL __NodeRGBA : public Node, public __RGBAProtocol
{
public:
    // overrides
    virtual GLubyte getOpacity() const override { return Node::getOpacity(); }
    virtual GLubyte getDisplayedOpacity() const  override { return Node::getDisplayedOpacity(); }
    virtual void setOpacity(GLubyte opacity) override { return Node::setOpacity(opacity); }
    virtual void updateDisplayedOpacity(GLubyte parentOpacity) override { return Node::updateDisplayedOpacity(parentOpacity); }
    virtual bool isCascadeOpacityEnabled() const  override { return Node::isCascadeOpacityEnabled(); }
    virtual void setCascadeOpacityEnabled(bool cascadeOpacityEnabled) override { return Node::setCascadeOpacityEnabled(cascadeOpacityEnabled); }

    virtual const Color3B& getColor(void) const override { return Node::getColor(); }
    virtual const Color3B& getDisplayedColor() const override { return Node::getDisplayedColor(); }
    virtual void setColor(const Color3B& color) override { return Node::setColor(color); }
    virtual void updateDisplayedColor(const Color3B& parentColor) override { return Node::updateDisplayedColor(parentColor); }
    virtual bool isCascadeColorEnabled() const override { return Node::isCascadeColorEnabled(); }
    virtual void setCascadeColorEnabled(bool cascadeColorEnabled) override { return Node::setCascadeColorEnabled(cascadeColorEnabled); }

    virtual void setOpacityModifyRGB(bool bValue) override { return Node::setOpacityModifyRGB(bValue); }
    virtual bool isOpacityModifyRGB() const override { return Node::isOpacityModifyRGB(); }

protected:
    __NodeRGBA();
    virtual ~__NodeRGBA() {}

private:
    CC_DISALLOW_COPY_AND_ASSIGN(__NodeRGBA);
};

// end of base_node group
/// @}

NS_CC_END

#endif // __CCNODE_H__
