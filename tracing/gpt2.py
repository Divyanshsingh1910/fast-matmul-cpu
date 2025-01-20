import torch
import intel_extension_for_pytorch as ipex
from transformers import AutoModelForCausalLM, AutoTokenizer
import os
from datetime import datetime

# Create directory for profiler logs
timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
log_dir = f"./log/gpt2_fixed_{timestamp}"
os.makedirs(log_dir, exist_ok=True)

try:
    # Load model and tokenizer
    model = AutoModelForCausalLM.from_pretrained("gpt2").eval()
    tokenizer = AutoTokenizer.from_pretrained("gpt2")

    # Create random input tensor
    input_ids = torch.randint(0, model.config.vocab_size, (1, 128), dtype=torch.long)
    attention_mask = torch.ones_like(input_ids)

    if tokenizer.pad_token_id is None:
        tokenizer.pad_token_id = tokenizer.eos_token_id

    # Optimize with IPEX
    model = ipex.optimize(
        model, 
        dtype=torch.float32,
        inplace=True,
        auto_kernel_selection=True
    )

    # Warmup
    print("Running warmup...")
    with torch.inference_mode():
        for _ in range(2):
            outputs = model.generate(
                input_ids,
                max_length=158,
                num_beams=4,
                attention_mask=attention_mask,
                pad_token_id=tokenizer.pad_token_id
            )
    
    # Actual profiling using autograd profiler
    print("Running profiling...")
    with torch.autograd.profiler.profile(
        use_cuda=False,
        profile_memory=True,
        record_shapes=True
    ) as prof:
        with torch.inference_mode():
            outputs = model.generate(
                input_ids,
                max_length=158,
                num_beams=4,
                attention_mask=attention_mask,
                pad_token_id=tokenizer.pad_token_id
            )

    # Save and print profiling results
    print("\nSaving profiling results...")
    
    # Print top operations by CPU time
    print("\nTop operations by CPU time:")
    print(prof.table(
        sort_by="cpu_time_total", 
        row_limit=20,
    ))

    # Save detailed profiling information
    with open(f"{log_dir}/detailed_profile.txt", "w") as f:
        f.write(prof.table())
        f.write("\n\nDetailed key averages:\n")
        f.write(str(prof.key_averages()))

    # Export chrome trace
    prof.export_chrome_trace(f"{log_dir}/trace.json")

    print(f"\nProfiling complete. Results saved to {log_dir}")

    # Print events specifically related to matmul operations
    print("\nMatrix multiplication operations:")
    for event in prof.function_events:
        if "matmul" in event.name.lower() or "linear" in event.name.lower():
            print(f"Operation: {event.name}")
            print(f"CPU time: {event.cpu_time_total/1000:.2f}ms")
            print(f"Input shapes: {event.input_shapes}")
            print("-" * 50)

except Exception as e:
    print(f"Error occurred: {str(e)}")
    raise