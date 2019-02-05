import cpp2py.clang_parser as CL
from synopsis import make_synopsis_list, make_synopsis_template_decl
from cpp2py.doc import make_table, replace_latex
from example import prepare_example

# common tools for both rendering functions

rst_start = """
..
   Generated automatically by cpp2rst

.. highlight:: c

"""

# The hidden toc for RST reference
toctree_hidden ="""
.. toctree::
    :hidden:
      
"""

def head(s): return '\n\n' + s + '\n' + '-'*len(s)  + '\n\n'

def escape_lg(s):
    return s.replace('>','\>').replace('<','\<')

def render_list(item, header):
    if len(item) > 0:
        R = head(header)
        for p,d in item: R += " * **%s**: %s\n"%(p,d) 
        return R
    else : return ''

def render_note(name, doc_elem) :
    """
    @param name warning or note
    """
    note = doc_elem[name]
    if note:
       return """
.. %s::
    %s"""%(name, note)
    else:
       return ''
    

def render_fig(doc_elem) : 
    figs = doc_elem['figure']
    fig = figs.split(":") if figs else (None, None)
    if figs:
        return """
.. figure:: {fig[0]}
   :alt: {fig[1]}
   :align: center
        """.format(fig = fig) + fig[1].lstrip(' \t\n\r') + '\n'
    else : return ''

#------------------------------------
def render_fnt(f_name, doc_methods, parent_class, f_overloads):
    R= rst_start

    parent_cls_fully_qualified = (CL.fully_qualified(parent_class.referenced) + "::") if parent_class else ""
    f_name_full = parent_cls_fully_qualified + f_name
    R += """
.. _{class_rst_ref}:

{f_name_full}
""".format(f_name_full = f_name_full, class_rst_ref = ((parent_class.spelling + '_') if parent_class else '' )+ f_name)

    R += '=' * (len(f_name_full)+0) + '\n' + """
**Synopsis**:

.. code-block:: c

"""
    # Synopsis
    R += make_synopsis_list(f_overloads, doc_methods) + '\n\n'
    
    # Enumerate all overloads
    for n, (m,doc) in enumerate(zip(f_overloads, doc_methods)):
        doc_elem = doc.elements
        
        num = '(%s)'%(n+1) if len(f_overloads)>1 else ''
        R += num + doc.brief_doc + '\n'

        R += replace_latex(render_note('note', doc_elem))
        R += replace_latex(render_note('warning', doc_elem))
        R += render_fig(doc_elem)
        R += render_list(doc_elem['tparam'], 'Template parameters')
        R += render_list(doc_elem['param'], 'Parameters')

        if doc_elem['return']: R += head('Return value') + doc_elem['return']
        if doc.processed_doc: R += head('Documentation') + replace_latex(doc.processed_doc)
        R += '\n\n' 

    # any example from the overloads
    for d in doc_methods:
        print d.elements
    example_file_name = reduce(lambda x,y : x or y, [ d.elements['example'] for d in doc_methods] + [f_name], '')  
    print example_file_name
    code,d1,d2, s,e = prepare_example(example_file_name, 4)
    if code:
        R += """
Example
---------

{d1}

.. triqs_example::

    linenos:{s},{e}

{code}

{d2}
        """.format(**locals())
    return R

#------------------------------------
def render_cls(cls, all_m, all_friend_functions, doc_methods, doc_class, cls_doc) : 
    doc_elem = doc_class.elements
    R= rst_start
    
    # include
    incl = doc_elem['include']

    # class header
    cls_fully_qualified = CL.fully_qualified(cls.referenced)
    R +="""
.. _{cls.spelling}:

{cls_fully_qualified}
{separator}

Defined in header <*{incl}*>

**Synopsis**:

.. code-block:: c

    {templ_synop}class {cls.spelling};

{cls_doc.brief_doc}

{cls_doc.processed_doc}
    """.format(cls = cls, incl = incl.strip(), separator = '=' * (len(cls_fully_qualified)), templ_synop = make_synopsis_template_decl(cls), cls_doc = cls_doc, cls_fully_qualified = cls_fully_qualified)

    # doc, note, warning, figure
    R += replace_latex(doc_class.processed_doc)
    R += replace_latex(render_note('note', doc_elem))
    R += replace_latex(render_note('warning', doc_elem))
    R += render_fig(doc_elem)
    R += render_list(doc_elem['tparam'], 'Template parameters')

    c_members = list(CL.get_members(cls, True)) 
    if len(c_members) > 0:
        R += head('Public members') 
        R += make_table([(t.spelling,t.type.spelling, replace_latex(t.raw_comment) if t.raw_comment else '') for t in c_members])

    c_usings = list(CL.get_usings(cls)) 
    if len(c_usings) > 0:
        R += head('Member types') 
        R += make_table([(t.spelling, replace_latex(t.raw_comment) if t.raw_comment else '') for t in c_usings])

    if len(all_m) > 0:
        R += head('Member functions') 
        R += make_table([(":ref:`%s <%s_%s>`"%(name,escape_lg(cls.spelling), escape_lg(name)), replace_latex(doc_methods[name][0].brief_doc)) for name in all_m])

        R += toctree_hidden
        for m_name in all_m:
           R += "    {cls.spelling}/{m_name}\n".format(cls = cls, m_name = m_name)

    if len(all_friend_functions) > 0:
        R += head('Non Member functions') 
        R += make_table([(":ref:`%s <%s_%s>`"%(name,escape_lg(cls.spelling), escape_lg(name)), replace_latex(doc_methods[name][0].brief_doc)) for name in all_friend_functions])
 
        R += toctree_hidden
        for f_name in all_friend_functions:
           R += "    {cls.spelling}/{f_name}\n".format(cls = cls, f_name = f_name)

    code,d1,d2, s,e = prepare_example(filename = cls.spelling + ".cpp", decal = 4)
    if code is not None:
        R +="""
Example
---------

{d1}

.. triqs_example::

    linenos:{s},{e}

{code}

{d2}
        """.format(d1 = d1, d2 = d2,  s = s, e = e, code = code)
    return R
