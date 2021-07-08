from collections import OrderedDict

from tensorflow.keras.utils import model_to_dot

class Node:
    def __init__(self, layer_name, layer_type):
        self.layer_name = layer_name
        self.layer_type = layer_type
        self.layer_attr = {}
        self.src = []
        self.dst = []
    
    def add_src(self, node):
        if node is None:
            return
        if node not in self.src:
            self.src.append(node)
        if self not in node.dst:
            node.dst.append(self)
        
    def add_dst(self, node):
        if node is None:
            return
        if node not in self.dst:
            self.dst.append(node)
        if self not in node.src:
            node.src.append(self)

    def set_layer_attr(self, key, value):
        self.layer_attr[key] = value

    def copy(self, node):
        self.layer_name = node.layer_name
        self.layer_type = node.layer_type
        self.layer_attr = node.layer_attr.copy()
        self.src = node.src.copy()
        self.dst = node.dst.copy()
        
    def delete(self):
        for node in self.src:
            node.dst.remove(self)

        for node in self.dst:
            node.src.remove(self)

    def print_node(self):
        print("{} -> ".format(self.layer_name), end='')
        for d in self.dst:
            print("{}, ".format(d.layer_name), end='')

class Graph:
    def __init__(self) -> None:
        self.root = None
        self.node_list = OrderedDict()

    def add_node(self, node) -> None:
        if self.root is None:
            self.root = node
        self.node_list[node.layer_name] = node

    def get_node(self, layer_name) -> Node:
        if layer_name in self.node_list.keys():
            return self.node_list[layer_name]
        else:
            return None

    def copy(self, graph) -> None:
        if graph.root is None:
            self.root = None
        else:
            self.root.copy(graph.root)
        self.node_list = graph.node_list.copy()

    def get_layer_names(self) -> list:
        return list(self.node_list.keys())

    def print_graph(self):
        print("\nPrinting graph...")
        for k in self.node_list:
            self.node_list[k].print_node()
            print()


def conv2matrix(weight_tensor, output_tensor):
    input_size = [ output_tensor[0]*output_tensor[1]*output_tensor[2], weight_tensor[0]*weight_tensor[1]*weight_tensor[2] ]
    weight_size = [ weight_tensor[0]*weight_tensor[1]*weight_tensor[2], weight_tensor[3] ]

    return input_size, weight_size

def dense2matrix(weight_tensor, output_tensor):
    assert len(weight_tensor) == 2, "Unconventional weight tensor"

    input_reuse = 1
    for d in range(len(output_tensor)-1):
        input_reuse = input_reuse * output_tensor[d] 

    input_size = [input_reuse, weight_tensor[0]]
    weight_size = [weight_tensor[0], weight_tensor[1]]
    
    return input_size, weight_size
    
def get_layer_info(node_label):
    return node_label.split(': ')

def convert_keras_to_graph(model):
    dot_graph = model_to_dot(model, show_shapes=False, show_layer_names=True, rankdir='TB', expand_nested=False, dpi=96)

    graph = Graph()

    for n in dot_graph.get_nodes():
        label = n.get_label()

        if label is not None:
            layer_name, layer_type = get_layer_info(label)
            graph.add_node(Node(layer_name, layer_type))

    for e in dot_graph.get_edges():
        src = e.get_source()
        src_node = dot_graph.get_node(src)[0]
        src_label = src_node.get_label()
        if src_label is None:
            continue
        src_layer_name, _ = get_layer_info(src_label)

        dst = e.get_destination()
        dst_node = dot_graph.get_node(dst)[0]
        dst_label = dst_node.get_label()
        dst_layer_name, _ = get_layer_info(dst_label)

        graph.get_node(dst_layer_name).add_src(graph.get_node(src_layer_name))

    graph_tmp = Graph()
    for layer_name in graph.get_layer_names():
        node = graph.get_node(layer_name)
        layer = model.get_layer(layer_name)

        if node is None:
            continue

        if node.layer_type == "Conv2D":
            weight_tensor = layer.get_weights()[0].shape
            output_tensor = layer.output_shape
            input_size, weight_size = conv2matrix(weight_tensor, output_tensor)

            node.set_layer_attr("input_size", input_size)
            node.set_layer_attr("weight_size", weight_size)

            graph_tmp.add_node(node)

        elif node.layer_type == "Dense":
            weight_tensor = layer.get_weights()[0].shape
            output_tensor = layer.output_shape
            input_size, weight_size = dense2matrix(weight_tensor, output_tensor)
            
            node.set_layer_attr("input_size", input_size)
            node.set_layer_attr("weight_size", weight_size)

            graph_tmp.add_node(node)

        elif node.layer_type == "FeedForward":
            node_dense1 = Node(layer_name+"1", "Dense")
            
            weight_tensor = layer.W1.get_shape().as_list()
            output_tensor = layer.output_shape
            input_size, weight_size = dense2matrix(weight_tensor, output_tensor)
            node_dense1.set_layer_attr("input_size", input_size)
            node_dense1.set_layer_attr("weight_size", weight_size)

            for s in node.src:
                node_dense1.add_src(s)

            node_dense2 = Node(layer_name+"2", "Dense")
            
            weight_tensor = layer.W2.get_shape().as_list()
            output_tensor = layer.output_shape
            input_size, weight_size = dense2matrix(weight_tensor, output_tensor)

            node_dense2.set_layer_attr("input_size", input_size)
            node_dense2.set_layer_attr("weight_size", weight_size)

            node_dense2.add_src(node_dense1)

            for d in node.dst:
                node_dense2.add_dst(d)

            graph_tmp.add_node(node_dense1)
            graph_tmp.add_node(node_dense2)

            node.delete()
        elif node.layer_type == "MultiHeadAttention":
            if isinstance(layer.input, list): #decoder query attention
                input_tensor = layer.input_shape[0]
                for i in range(1,len(layer.input_shape)):
                    assert input_tensor == layer.input_shape[i]
            else: #self-attention
                input_tensor = layer.input_shape

            batch_size = input_tensor[0]
            sentence_len = input_tensor[1]
            d_model = input_tensor[2]

            node_K = Node(layer_name+"_K", "Dense")

            Wk = layer.Wk.get_shape().as_list()
            input_size, weight_size = [[batch_size*sentence_len, d_model],list(Wk)]
            node_K.set_layer_attr("input_size", input_size)
            node_K.set_layer_attr("weight_size", weight_size)

            for s in node.src:
                node_K.add_src(s)

            node_Q = Node(layer_name+"_Q", "Dense")

            Wq = layer.Wq.get_shape().as_list()
            input_size, weight_size = [[batch_size*sentence_len, d_model],list(Wq)]
            node_Q.set_layer_attr("input_size", input_size)
            node_Q.set_layer_attr("weight_size", weight_size)

            for s in node.src:
                node_Q.add_src(s)

            node_V = Node(layer_name+"_V", "Dense")

            Wv = layer.Wv.get_shape().as_list()
            input_size, weight_size = [[batch_size*sentence_len, d_model],list(Wv)]
            node_V.set_layer_attr("input_size", input_size)
            node_V.set_layer_attr("weight_size", weight_size)

            for s in node.src:
                node_V.add_src(s)

            d_h = int(d_model/layer.head_num)

            node_QK = []
            for i in range(layer.head_num):
                n = Node(layer_name+"_QK"+str(i), "Dense")

                input_size, weight_size = [[input_tensor[0]*input_tensor[1], d_h], [d_h, input_tensor[0]*input_tensor[1]]]
                n.set_layer_attr("input_size", input_size)
                n.set_layer_attr("weight_size", weight_size)

                n.add_src(node_K)
                n.add_src(node_Q)
                n.add_src(node_V)

                node_QK.append(n)

            node_QK_concat = Node(layer_name+"_QK-Concat", "Concat")
            for i in range(layer.head_num):
                node_QK_concat.add_src(node_QK[i])
            
            node_PV = []
            for i in range(layer.head_num):
                n = Node(layer_name+"_PV"+str(i), "Dense")

                input_size, weight_size = [[input_tensor[0]*input_tensor[1], input_tensor[0]*input_tensor[1]], [input_tensor[0]*input_tensor[1], d_h]]
                n.set_layer_attr("input_size", input_size)
                n.set_layer_attr("weight_size", weight_size)

                n.add_src(node_QK_concat)

                node_PV.append(n)  

            node_PV_concat = Node(layer_name+"_PV-Concat", "Concat")
            for i in range(layer.head_num):
                node_PV_concat.add_src(node_PV[i])

            node_O = Node(layer_name+"_O", "Dense")

            Wo = layer.Wo.get_shape().as_list()
            input_size, weight_size = [[batch_size*sentence_len, d_model],list(Wo)]
            node_O.set_layer_attr("input_size", input_size)
            node_O.set_layer_attr("weight_size", weight_size)

            node_O.add_src(node_PV_concat)

            for d in node.dst:
                node_O.add_dst(d)

            graph_tmp.add_node(node_K)
            graph_tmp.add_node(node_Q)
            graph_tmp.add_node(node_V)
            for i in range(layer.head_num):
                graph_tmp.add_node(node_QK[i])
            graph_tmp.add_node(node_QK_concat)
            for i in range(layer.head_num):
                graph_tmp.add_node(node_PV[i])
            graph_tmp.add_node(node_PV_concat)
            graph_tmp.add_node(node_O)

            node.delete()
        else:
            graph_tmp.add_node(node)

    graph.copy(graph_tmp)
    #graph.print_graph()

    return graph