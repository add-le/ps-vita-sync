#ifndef _CLOUD_HPP_
#define _CLOUD_HPP_

#define pure =0

class ICloud {
  protected:
    virtual void download() pure;
    virtual void upload() pure;
}

#endif

