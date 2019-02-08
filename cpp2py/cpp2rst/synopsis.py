import re, os
import cpp2py.clang_parser as CL

def decay(s) :
    for tok in ['const ', 'const&', '&&', '&'] :
        s = re.sub(tok,'',s)
    return s.strip()

def make_synopsis_template_decl(node):
    """
        Given a node (class, function, method), 
        extract its template parameters 
        and return the string template<...>
    """ 
    tparams = CL.get_template_params(node)
    if not tparams: return ''
    targs = ', '.join("%s %s"%(pp[0],pp[1]) + (" = %s"%pp[2] if (len(pp)==3 and pp[2]) else '') for pp in tparams)
    return "template<%s> "%targs

shift = 4
maxlen = 120

# def reindent(s, shift):
    # if shift >= 0 : return '\n'.join(shift*' ' + x for x in s.split('\n'))
    # return '\n'.join(x[shift:] for x in s.split('\n'))


def process_param_type(t_name, remove):
    t_name = re.sub(remove, '', t_name)
    if t_name in class_list_name: # has a link
       d = decay(t_name)
       return t_name.replace(d,":ref:`%s <%s>`"%(d,d))
    else:
       return t_name

def process_rtype(t_name, remove):
    t_name = re.sub(remove, '', t_name)
    tname =  re.sub(r"\s*typename\s+std\d*::enable_if<(.*),(.*)>::type", r"requires(\1)\n \2 ", t_name)
    return tname

def make_synopsis_one_function(f):
    """
        Given the AST node for a function f, returns the synopsis
    """
    # If @synopsis was given manually
    #syn = f.processed_doc.elements.pop('synopsis', '')
    #if syn : return [syn]
 
    ns = CL.get_namespace(f) + '::' # to remove the myclass:: from the types of arg and rtype 
    is_not_constructor = not getattr(f, 'is_constructor', False)
    
    template = make_synopsis_template_decl(f)
    
    result_type = process_rtype(f.result_type.spelling, remove = ns) if is_not_constructor else ''
    name = " %s "%f.spelling.strip() if is_not_constructor else f.spelling.split('<',1)[0] # eliminate the <> in the constructor name
    qualif = CL.get_method_qualification(f) + (' noexcept' if getattr(f,'noexcept',False) else '')
   
    params1 = [(p.type.spelling, p.spelling, CL.get_param_default_value(p)) for p in CL.get_params(f)]
    params = ["%s %s"%(process_param_type(t, remove = ns),n) + (" = %s"%d if d else "") for t,n,d in params1]
  
    res = result_type + name + '('
    l = len(res)
    for x in params : 
        if l + len(x) > maxlen : 
            res += '\n' + 9*' '
            l = 9
        l +=len(x)
        res += x + ', '
    if params : res = res[:-2] #eliminate last ,
    return (template + '\n' + 7*' ' if template else '') + res + ') ' + qualif


def make_synopsis_list(f_list):
    return '  ' + '\n\n  '.join("(%s) %s"%(n,make_synopsis_one_function(f)) for n, f in enumerate(f_list))


