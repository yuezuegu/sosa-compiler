from typing import Type, Any, Dict, Tuple, List
from dataclasses import fields

def named_zip(**kwargs: Dict[str, List[Any]]) -> List[Dict[str, Any]]:
    return [
        dict(zip(kwargs.keys(), v)) for v in zip(*kwargs.values())
    ]

# TODO is there a better way to create dataclasses from dictionaries?
def _from_dict():
    # TODO support nested dataclasses as well.
    def construct_from(t: Type, o: Any):
        if o is None:
            return None
        
        if hasattr(t, "__origin__"):
            t = t.__origin__

        if isinstance(o, t):
            return o
        
        if t is Tuple:
            return tuple(o)
        
        if t is Dict:
            return dict(o)
        
        return t(o)
    
    def from_dict(c: Type):
        """
        A simple decorator that adds `from_dict` to dataclasses.
        """
        def f(d: Dict[str, Any]):
            return c(**{ f.name: construct_from(f.type, d.get(f.name)) for f in fields(c) })
        c.from_dict = f
        return c
    return from_dict

from_dict = _from_dict()
del _from_dict
