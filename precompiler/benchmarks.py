
from tensorflow.keras.applications.resnet import ResNet50
from tensorflow.keras.applications.resnet import ResNet101
from tensorflow.keras.applications.resnet import ResNet152
from tensorflow.keras.applications.densenet import DenseNet121
from tensorflow.keras.applications.densenet import DenseNet169
from tensorflow.keras.applications.densenet import DenseNet201
from tensorflow.keras.applications.inception_v3 import InceptionV3
from tensorflow.keras.layers import Input
from keras_bert.bert import get_model,compile_model
from filelock import FileLock

import pickle

import time 

from .graph import convert_keras_to_graph

class benchmark:
	def __init__(self, model_name, model_type, image_size=299, seq_len=128, batch_size=1):
		print("Constructing the model: ", model_name)
		self.model_name = model_name
		self.model_type = model_type

		self.batch_size = batch_size
		self.image_size = image_size
		self.seq_len = seq_len

		if self.model_type == "CNN":
			assert self.image_size is not None, "CNN models must have image size."
		elif self.model_type == "BERT":
			assert self.seq_len is not None, "BERT models must have sentence length."
		else:
			raise NotImplementedError

		if self.model_name == 'bert_tiny':
			self.model_size = 256
			self.no_layers = 4

		elif self.model_name == 'bert_small':
			self.model_size = 512
			self.no_layers = 4
			
		elif self.model_name == 'bert_medium':
			self.model_size = 512
			self.no_layers = 8

		elif self.model_name == 'bert_base':
			self.model_size = 768
			self.no_layers = 12

		elif self.model_name == 'bert_large':
			self.model_size = 1024
			self.no_layers = 24

		self.layer_dims = self.calculate_layer_dims()

		self.no_ops = 0
		for layer_name in self.layer_dims:
			X = self.layer_dims[layer_name][0]
			W = self.layer_dims[layer_name][1]
			self.no_ops += (2 * X[0] * X[1] * W[1])

	def get_keras_model(self, no_layers=None):
		if self.model_name == 'inception':
			return InceptionV3(input_tensor = Input(batch_shape=(self.batch_size, self.image_size, self.image_size, 3)), weights='imagenet')
		elif self.model_name == 'resnet50':
			return ResNet50(input_tensor = Input(batch_shape=(self.batch_size, self.image_size, self.image_size, 3)), weights='imagenet')
		elif self.model_name == 'resnet101':
			return ResNet101(input_tensor = Input(batch_shape=(self.batch_size, self.image_size, self.image_size, 3)), weights='imagenet')
		elif self.model_name == 'resnet152':
			return ResNet152(input_tensor = Input(batch_shape=(self.batch_size, self.image_size, self.image_size, 3)), weights='imagenet')
		elif self.model_name == 'densenet121':
			return DenseNet121(input_tensor = Input(batch_shape=(self.batch_size, self.image_size, self.image_size, 3)), weights='imagenet')
		elif self.model_name == 'densenet169':
			return DenseNet169(input_tensor = Input(batch_shape=(self.batch_size, self.image_size, self.image_size, 3)), weights='imagenet')
		elif self.model_name == 'densenet201':
			return DenseNet201(input_tensor = Input(batch_shape=(self.batch_size, self.image_size, self.image_size, 3)), weights='imagenet')
		elif self.model_type == "BERT":
			model_size = self.model_size
			
			if no_layers is None:
				no_layers = self.no_layers

			model = get_model(
				token_num=30000,
				head_num=int(model_size/64),
				transformer_num=no_layers,
				embed_dim=model_size,
				feed_forward_dim=4*model_size,
				batch_size=self.batch_size,
				seq_len=self.seq_len,
				pos_num=self.seq_len,
			)
			compile_model(model)
			#model.summary()
			return model

		else:
			raise NotImplementedError()

	def calculate_layer_dims(self):
		keras_model = self.get_keras_model()

		graph = convert_keras_to_graph(keras_model)

		mat_sizes = {}
		for layer_name in graph.get_layer_names():
			layer_node = graph.get_node(layer_name)

			if layer_node.layer_type == 'Conv2D' or layer_node.layer_type == 'Dense':
				input_size = layer_node.layer_attr["input_size"]
				weight_size = layer_node.layer_attr["weight_size"]

				mat_sizes[layer_name] = [input_size, weight_size]

		return mat_sizes

def create_benchmarks():
	benchmarks = {}
	
	batch_size = 1
	
	benchmarks["inception"] = {batch_size: {}}
	benchmarks["resnet50"] = {batch_size: {}}
	benchmarks["resnet101"] = {batch_size: {}}
	benchmarks["resnet152"] = {batch_size: {}}
	benchmarks["densenet121"] = {batch_size: {}}
	benchmarks["densenet169"] = {batch_size: {}}
	benchmarks["densenet201"] = {batch_size: {}}
	for image_size in [224, 256, 299]:
		benchmarks["inception"][batch_size][image_size] = benchmark("inception", "CNN", batch_size=batch_size, image_size=image_size)
		benchmarks["resnet50"][batch_size][image_size] = benchmark("resnet50", "CNN", batch_size=batch_size, image_size=image_size)
		benchmarks["resnet101"][batch_size][image_size] = benchmark("resnet101", "CNN", batch_size=batch_size, image_size=image_size)
		benchmarks["resnet152"][batch_size][image_size] = benchmark("resnet152", "CNN", batch_size=batch_size, image_size=image_size)
		benchmarks["densenet121"][batch_size][image_size] = benchmark("densenet121", "CNN", batch_size=batch_size, image_size=image_size)
		benchmarks["densenet169"][batch_size][image_size] = benchmark("densenet169", "CNN", batch_size=batch_size, image_size=image_size)
		benchmarks["densenet201"][batch_size][image_size] = benchmark("densenet201", "CNN", batch_size=batch_size, image_size=image_size)


	batch_size = 1
	benchmarks["bert_tiny"] = {batch_size: {}}
	benchmarks["bert_small"] = {batch_size: {}}
	benchmarks["bert_medium"] = {batch_size: {}}
	benchmarks["bert_base"] = {batch_size: {}}
	benchmarks["bert_large"] = {batch_size: {}}
	for seq_len in [10, 20, 40, 60, 80, 100, 200, 300, 400, 500]:
		benchmarks["bert_tiny"][batch_size][seq_len] = benchmark("bert_tiny", "BERT", seq_len=seq_len, batch_size=1)
		benchmarks["bert_small"][batch_size][seq_len] = benchmark("bert_small", "BERT", seq_len=seq_len, batch_size=1)
		benchmarks["bert_medium"][batch_size][seq_len] = benchmark("bert_medium", "BERT", seq_len=seq_len, batch_size=1)
		benchmarks["bert_base"][batch_size][seq_len] = benchmark("bert_base", "BERT", seq_len=seq_len, batch_size=1)
		benchmarks["bert_large"][batch_size][seq_len] = benchmark("bert_large", "BERT", seq_len=seq_len, batch_size=1)

	pickle.dump(benchmarks, open("benchmarks.pickle", 'wb'))

def get_benchmarks(model_name, batch_size, image_size, seq_len):
	with FileLock("benchmarks.pickle.lock"):
		# print("Lock acquired for benchmarks.pickle")

		try:
			pickle_file = open('benchmarks.pickle', 'rb')
		
			benchmarks = pickle.load(pickle_file)
		
			if "bert" in model_name:
				if model_name in benchmarks:
					if batch_size in benchmarks[model_name]:
						if seq_len in benchmarks[model_name][batch_size]:
							return benchmarks[model_name][batch_size][seq_len]
				bm = benchmark(model_name, "BERT", seq_len=seq_len, batch_size=batch_size)
				benchmarks[model_name][batch_size][seq_len] = bm
			else:
				if model_name in benchmarks:
					if batch_size in benchmarks[model_name]:
						if image_size in benchmarks[model_name][batch_size]:
							return benchmarks[model_name][batch_size][image_size]
				bm = benchmark(model_name,"CNN",batch_size=batch_size, image_size=image_size)
				benchmarks[model_name][batch_size][image_size] = bm

		except:
			benchmarks = {}
			benchmarks[model_name] = {}
			benchmarks[model_name][batch_size] = {}
			if "bert" in model_name:
				bm = benchmark(model_name, "BERT", seq_len=seq_len, batch_size=batch_size)
				benchmarks[model_name][batch_size][seq_len] = bm
			else:
				bm = benchmark(model_name, "CNN", batch_size=batch_size, image_size=image_size)
				benchmarks[model_name][batch_size][image_size] = bm

		with open("benchmarks.pickle", 'wb') as pkl_file:
			pickle.dump(benchmarks, pkl_file)

	return bm

if __name__ == "__main__":
	start = time.time()
	create_benchmarks()
	print("Done in " + "{:.1f}  minutes. ".format((time.time() - start)/60))
