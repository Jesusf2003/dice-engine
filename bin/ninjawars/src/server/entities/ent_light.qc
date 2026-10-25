class light : BaseEntity
{
  void() light = {
    remove(this);
  };
};

class light_spot : light {};
class light_sun : light {};